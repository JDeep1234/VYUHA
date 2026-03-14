/*
 * ============================================================================
 * ML BRIDGE - C++ <-> Python Subprocess implementation
 * ============================================================================
 * Authors: Karthik (ML) + Jdeep (integration bridge)
 *
 * Launches ml_server.py as a child process and communicates over its
 * stdin/stdout with newline-delimited JSON.
 *
 * Windows-specific: uses CreateProcess + anonymous pipes.
 * ============================================================================
 */

#include "ml_framework/ml_bridge.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <sstream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <io.h> // _open_osfhandle, _O_RDONLY
#include <windows.h>
#undef max
#undef min
#else
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace edr {
namespace ml {

// ============================================================================
// SystemState -> 30-element float vector
// ============================================================================

std::vector<float> SystemState::toVector() const {
  std::vector<float> v;
  v.reserve(30);

  // EDR product one-hot (5)
  for (float f : edrProduct)
    v.push_back(f);
  // EDR state (3)
  v.push_back(edrVersion);
  v.push_back(edrProcessRunning);
  v.push_back(edrDriverLoaded);
  // Windows version one-hot (4)
  for (float f : windowsVersion)
    v.push_back(f);
  // System config (8)
  v.push_back(windowsBuild);
  for (float f : integrityLevel)
    v.push_back(f);
  v.push_back(pplProtection);
  v.push_back(driverSigEnforcement);
  v.push_back(secureBoot);
  v.push_back(virtualization);
  v.push_back(antimalwareLight);
  // Previous actions (8 one-hot)
  for (float f : lastAction)
    v.push_back(f);
  // Previous result (4 one-hot)
  for (float f : lastActionResult)
    v.push_back(f);
  // Counters (2)
  v.push_back(consecutiveFailures);
  v.push_back(timeSinceLastAction);

  // Ensure exactly 30 elements
  while (v.size() < 30)
    v.push_back(0.f);
  if (v.size() > 30)
    v.resize(30);
  return v;
}

SystemState SystemState::fromContext(const std::string &edrName,
                                     const std::string & /*edrVer*/,
                                     bool edrRunning, bool driverLoaded,
                                     int winBuild, int intLevel, bool ppl,
                                     bool dse, bool sb, bool virt, bool elam,
                                     int lastActionId, int lastResultId,
                                     int consecFails, double secsSinceLast) {
  SystemState s;

  // EDR product one-hot
  static const char *EDR_NAMES[] = {"defender", "crowdstrike", "carbonblack",
                                    "sophos", "other"};
  std::string lower = edrName;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  bool matched = false;
  for (int i = 0; i < 4; i++) {
    if (lower.find(EDR_NAMES[i]) != std::string::npos) {
      s.edrProduct[i] = 1.f;
      matched = true;
      break;
    }
  }
  if (!matched)
    s.edrProduct[4] = 1.f;

  // EDR state
  s.edrProcessRunning = edrRunning ? 1.f : 0.f;
  s.edrDriverLoaded = driverLoaded ? 1.f : 0.f;

  // Windows version from build number
  if (winBuild >= 22000)
    s.windowsVersion[1] = 1.f; // Win11
  else if (winBuild >= 19041)
    s.windowsVersion[0] = 1.f; // Win10
  else if (winBuild >= 17763)
    s.windowsVersion[2] = 1.f; // Server 2019
  else
    s.windowsVersion[3] = 1.f; // Server 2022+/other

  // Build normalised (max ~25000 for future-proofing)
  s.windowsBuild = std::min(1.f, static_cast<float>(winBuild) / 25000.f);

  // Integrity level one-hot (0=Medium, 1=High, 2=System)
  if (intLevel >= 0 && intLevel <= 2)
    s.integrityLevel[intLevel] = 1.f;

  s.pplProtection = ppl ? 1.f : 0.f;
  s.driverSigEnforcement = dse ? 1.f : 0.f;
  s.secureBoot = sb ? 1.f : 0.f;
  s.virtualization = virt ? 1.f : 0.f;
  s.antimalwareLight = elam ? 1.f : 0.f;

  // Last action one-hot
  if (lastActionId >= 0 && lastActionId < 8)
    s.lastAction[lastActionId] = 1.f;

  // Last result one-hot
  if (lastResultId >= 0 && lastResultId < 4)
    s.lastActionResult[lastResultId] = 1.f;

  s.consecutiveFailures = std::min(1.f, static_cast<float>(consecFails) / 8.f);
  s.timeSinceLastAction =
      std::min(1.f, static_cast<float>(secsSinceLast) / 300.f);

  return s;
}

// ============================================================================
// Simple JSON helpers (no external library)
// ============================================================================

std::string MLBridge::vectorToJsonArray(const std::vector<float> &v) {
  std::ostringstream oss;
  oss << "[";
  for (size_t i = 0; i < v.size(); ++i) {
    if (i > 0)
      oss << ",";
    oss << v[i];
  }
  oss << "]";
  return oss.str();
}

std::string MLBridge::mapToJsonObject(const std::map<std::string, float> &m) {
  std::ostringstream oss;
  oss << "{";
  bool first = true;
  for (const auto &[k, v] : m) {
    if (!first)
      oss << ",";
    oss << "\"" << k << "\":" << v;
    first = false;
  }
  oss << "}";
  return oss.str();
}

// Minimal JSON string-value reader (not full parser — handles flat objects).
std::string MLBridge::jsonGetString(const std::string &json,
                                    const std::string &key) {
  std::string search = "\"" + key + "\":\"";
  auto pos = json.find(search);
  if (pos == std::string::npos)
    return "";
  pos += search.size();
  auto end = json.find('"', pos);
  return (end != std::string::npos) ? json.substr(pos, end - pos) : "";
}

float MLBridge::jsonGetFloat(const std::string &json, const std::string &key) {
  std::string search = "\"" + key + "\":";
  auto pos = json.find(search);
  if (pos == std::string::npos)
    return 0.f;
  pos += search.size();
  // skip whitespace
  while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'))
    ++pos;
  try {
    return std::stof(json.substr(pos));
  } catch (...) {
    return 0.f;
  }
}

int MLBridge::jsonGetInt(const std::string &json, const std::string &key) {
  return static_cast<int>(std::lround(jsonGetFloat(json, key)));
}

// ============================================================================
// Process management
// ============================================================================

MLBridge::MLBridge() = default;

MLBridge::~MLBridge() { stop(); }

bool MLBridge::start(const std::string &pythonPath,
                     const std::string &scriptPath) {
  if (running_)
    return true;

  // Resolve script path
  std::string script = scriptPath;
  if (script.empty()) {
    // Try relative to executable: ../../ml_framework/python/ml_server.py
    // or sibling directory structures used in the build layout
    const std::vector<std::string> candidates = {
        "src/ml_framework/python/ml_server.py",
        "../src/ml_framework/python/ml_server.py",
        "../../src/ml_framework/python/ml_server.py",
        "../../../src/ml_framework/python/ml_server.py",
    };
    for (const auto &c : candidates) {
      if (std::filesystem::exists(c)) {
        script = c;
        break;
      }
    }
    if (script.empty()) {
      std::cerr << "[MLBridge] Could not find ml_server.py. "
                   "Set scriptPath explicitly."
                << std::endl;
      return false;
    }
  }

  std::string cmd = pythonPath + " \"" + script + "\"";
  std::cout << "[MLBridge] Starting Python bridge: " << cmd << std::endl;

#ifdef _WIN32
  HANDLE hChildStdin_Rd = NULL, hChildStdin_Wr = NULL;
  HANDLE hChildStdout_Rd = NULL, hChildStdout_Wr = NULL;

  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&hChildStdin_Rd, &hChildStdin_Wr, &sa, 0) ||
      !CreatePipe(&hChildStdout_Rd, &hChildStdout_Wr, &sa, 0)) {
    std::cerr << "[MLBridge] Failed to create pipes." << std::endl;
    return false;
  }

  // Ensure write end of stdin and read end of stdout are NOT inherited
  SetHandleInformation(hChildStdin_Wr, HANDLE_FLAG_INHERIT, 0);
  SetHandleInformation(hChildStdout_Rd, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOA si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  ZeroMemory(&pi, sizeof(pi));
  si.cb = sizeof(si);
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdInput = hChildStdin_Rd;
  si.hStdOutput = hChildStdout_Wr;
  si.hStdError = hChildStdout_Wr; // redirect stderr to stdout for logging

  std::string cmdCopy = cmd;
  if (!CreateProcessA(NULL, &cmdCopy[0], NULL, NULL, TRUE, CREATE_NO_WINDOW,
                      NULL, NULL, &si, &pi)) {
    DWORD err = GetLastError();
    std::cerr << "[MLBridge] CreateProcess failed: " << err << std::endl;
    return false;
  }

  processHandle_ = pi.hProcess;
  CloseHandle(pi.hThread);
  CloseHandle(hChildStdin_Rd);
  CloseHandle(hChildStdout_Wr);

  // Open CRT FILE* wrappers around the handles
  int fdIn = _open_osfhandle(reinterpret_cast<intptr_t>(hChildStdin_Wr), 0);
  int fdOut =
      _open_osfhandle(reinterpret_cast<intptr_t>(hChildStdout_Rd), _O_RDONLY);
  toChild_ = _fdopen(fdIn, "wb");
  fromChild_ = _fdopen(fdOut, "rb");

#else
  // POSIX popen2-style pipe
  int stdinPipe[2], stdoutPipe[2];
  pipe(stdinPipe);
  pipe(stdoutPipe);

  childPid_ = fork();
  if (childPid_ == 0) {
    // child
    dup2(stdinPipe[0], STDIN_FILENO);
    dup2(stdoutPipe[1], STDOUT_FILENO);
    dup2(stdoutPipe[1], STDERR_FILENO);
    close(stdinPipe[0]);
    close(stdinPipe[1]);
    close(stdoutPipe[0]);
    close(stdoutPipe[1]);
    execlp(pythonPath.c_str(), pythonPath.c_str(), script.c_str(), nullptr);
    _exit(1);
  }
  close(stdinPipe[0]);
  close(stdoutPipe[1]);
  toChild_ = fdopen(stdinPipe[1], "wb");
  fromChild_ = fdopen(stdoutPipe[0], "rb");
#endif

  if (!toChild_ || !fromChild_) {
    std::cerr << "[MLBridge] Failed to open pipe streams." << std::endl;
    return false;
  }

  running_ = true;

  // Health-check
  auto resp = sendReceive("{\"command\":\"ping\"}");
  if (resp.find("pong") == std::string::npos) {
    std::cerr << "[MLBridge] Health check failed: " << resp << std::endl;
    stop();
    return false;
  }

  std::cout << "[MLBridge] Python ML server ready." << std::endl;
  return true;
}

void MLBridge::stop() {
  if (!running_)
    return;

  if (toChild_) {
    fclose(toChild_);
    toChild_ = nullptr;
  }
  if (fromChild_) {
    fclose(fromChild_);
    fromChild_ = nullptr;
  }

#ifdef _WIN32
  if (processHandle_) {
    TerminateProcess(reinterpret_cast<HANDLE>(processHandle_), 0);
    CloseHandle(reinterpret_cast<HANDLE>(processHandle_));
    processHandle_ = nullptr;
  }
#else
  if (childPid_ > 0) {
    kill(childPid_, SIGTERM);
    waitpid(childPid_, nullptr, 0);
    childPid_ = -1;
  }
#endif

  running_ = false;
  std::cout << "[MLBridge] Python ML server stopped." << std::endl;
}

// ============================================================================
// Core send/receive
// ============================================================================

std::string MLBridge::sendReceive(const std::string &jsonMsg) {
  if (!running_ || !toChild_ || !fromChild_) {
    return "{\"error\":\"bridge_not_running\"}";
  }

  // Write message + newline
  std::string line = jsonMsg + "\n";
  if (fputs(line.c_str(), toChild_) == EOF || fflush(toChild_) != 0) {
    std::cerr << "[MLBridge] Write to Python process failed." << std::endl;
    return "{\"error\":\"write_failed\"}";
  }

  // Read response line
  char buf[65536];
  if (fgets(buf, sizeof(buf), fromChild_) == nullptr) {
    std::cerr << "[MLBridge] Read from Python process failed." << std::endl;
    return "{\"error\":\"read_failed\"}";
  }

  std::string response(buf);
  // Strip trailing newline
  while (!response.empty() &&
         (response.back() == '\n' || response.back() == '\r'))
    response.pop_back();

  return response;
}

// ============================================================================
// DQN interface
// ============================================================================

ActionResponse MLBridge::selectAction(const SystemState &state,
                                      const std::vector<int> &validActions) {
  auto vec = state.toVector();
  std::ostringstream oss;
  oss << "{\"command\":\"select_action\",\"state\":" << vectorToJsonArray(vec);
  if (!validActions.empty()) {
    oss << ",\"valid_actions\":[";
    for (size_t i = 0; i < validActions.size(); i++) {
      if (i > 0)
        oss << ",";
      oss << validActions[i];
    }
    oss << "]";
  }
  oss << "}";

  auto resp = sendReceive(oss.str());

  ActionResponse ar;
  ar.actionId = jsonGetInt(resp, "action");
  ar.actionName = jsonGetString(resp, "action_name");
  ar.valid = (resp.find("error") == std::string::npos);
  return ar;
}

TrainResponse MLBridge::train(const SystemState &state, int action,
                              float reward, const SystemState &nextState,
                              bool done) {
  auto sv = state.toVector();
  auto nsv = nextState.toVector();

  std::ostringstream oss;
  oss << "{\"command\":\"train\""
      << ",\"state\":" << vectorToJsonArray(sv) << ",\"action\":" << action
      << ",\"reward\":" << reward
      << ",\"next_state\":" << vectorToJsonArray(nsv)
      << ",\"done\":" << (done ? "true" : "false") << "}";

  auto resp = sendReceive(oss.str());

  TrainResponse tr;
  tr.loss = jsonGetFloat(resp, "loss");
  tr.epsilon = jsonGetFloat(resp, "epsilon");
  tr.valid = (resp.find("error") == std::string::npos);
  return tr;
}

bool MLBridge::updateTargetNetwork() {
  auto resp = sendReceive("{\"command\":\"update_target\"}");
  return resp.find("ok") != std::string::npos;
}

bool MLBridge::saveModel(const std::string &path) {
  std::ostringstream oss;
  oss << "{\"command\":\"save_model\",\"path\":\"" << path << "\"}";
  auto resp = sendReceive(oss.str());
  return resp.find("ok") != std::string::npos;
}

bool MLBridge::loadModel(const std::string &path) {
  std::ostringstream oss;
  oss << "{\"command\":\"load_model\",\"path\":\"" << path << "\"}";
  auto resp = sendReceive(oss.str());
  return resp.find("ok") != std::string::npos;
}

// ============================================================================
// BehaviorAnalyzer interface
// ============================================================================

ClusterResponse
MLBridge::clusterEDR(const std::map<std::string, float> &features,
                     const std::string &edrName) {
  std::ostringstream oss;
  oss << "{\"command\":\"cluster\""
      << ",\"edr_name\":\"" << edrName << "\""
      << ",\"features\":" << mapToJsonObject(features) << "}";

  auto resp = sendReceive(oss.str());

  ClusterResponse cr;
  cr.clusterId = jsonGetInt(resp, "cluster_id");
  cr.clusterName = jsonGetString(resp, "cluster_name");
  cr.valid = (resp.find("error") == std::string::npos);
  // Parse vulnerabilities array  (simple extraction)
  auto start = resp.find("\"vulnerabilities\":[");
  if (start != std::string::npos) {
    start += 18;
    auto end = resp.find(']', start);
    std::string arrayStr = resp.substr(start, end - start);
    // Each element: "text"
    size_t p = 0;
    while ((p = arrayStr.find('"', p)) != std::string::npos) {
      size_t q = arrayStr.find('"', p + 1);
      if (q == std::string::npos)
        break;
      cr.vulnerabilities.push_back(arrayStr.substr(p + 1, q - p - 1));
      p = q + 1;
    }
  }
  return cr;
}

bool MLBridge::updateBehavior(const std::string &edrName,
                              const std::map<std::string, float> &features) {
  std::ostringstream oss;
  oss << "{\"command\":\"update_behavior\""
      << ",\"edr_name\":\"" << edrName << "\""
      << ",\"features\":" << mapToJsonObject(features) << "}";
  auto resp = sendReceive(oss.str());
  return resp.find("ok") != std::string::npos;
}

// ============================================================================
// ExplainableAI interface
// ============================================================================

ExplainResponse MLBridge::explain(const std::string &techniqueId,
                                  const std::string &edrName,
                                  const SystemState &state,
                                  const std::string &outcome) {
  auto sv = state.toVector();
  std::ostringstream oss;
  oss << "{\"command\":\"explain\""
      << ",\"technique_id\":\"" << techniqueId << "\""
      << ",\"edr_name\":\"" << edrName << "\""
      << ",\"state\":" << vectorToJsonArray(sv) << ",\"outcome\":\"" << outcome
      << "\""
      << "}";

  auto resp = sendReceive(oss.str());

  ExplainResponse er;
  er.explanation = jsonGetString(resp, "explanation");
  er.narrative = jsonGetString(resp, "narrative");
  er.valid = (resp.find("\"valid\":true") != std::string::npos);

  // Parse top_features array: [["name", 0.42], ...]
  auto start = resp.find("\"top_features\":");
  if (start != std::string::npos) {
    start += 15;
    auto end = resp.find("]]", start);
    if (end != std::string::npos) {
      std::string arr = resp.substr(start, end - start + 2);
      size_t p = 0;
      while ((p = arr.find('[', p)) != std::string::npos) {
        size_t q = arr.find(',', p);
        size_t r = arr.find(']', q);
        if (q == std::string::npos || r == std::string::npos)
          break;
        std::string nameRaw = arr.substr(p + 1, q - p - 1);
        std::string scoreRaw = arr.substr(q + 1, r - q - 1);
        // Strip quotes
        nameRaw.erase(std::remove(nameRaw.begin(), nameRaw.end(), '"'),
                      nameRaw.end());
        try {
          float score = std::stof(scoreRaw);
          er.topFeatures.emplace_back(nameRaw, score);
        } catch (...) {
        }
        p = r + 1;
      }
    }
  }
  return er;
}

// ============================================================================
// Reward Calculator
// ============================================================================

namespace reward {

Outcome classifyOutcome(bool success, bool detected,
                        const std::string &output) {
  std::string lower = output;
  std::transform(lower.begin(), lower.end(), lower.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  bool crashed = lower.find("crash") != std::string::npos ||
                 lower.find("bsod") != std::string::npos ||
                 lower.find("bugcheck") != std::string::npos;

  if (crashed)
    return Outcome::SYSTEM_CRASH;
  if (!success && detected)
    return Outcome::ACTION_BLOCKED;
  if (!success)
    return Outcome::NO_CHANGE;
  if (success && detected)
    return Outcome::PARTIAL_SUCCESS; // ran but EDR saw it

  bool telemetry = lower.find("telemetry") != std::string::npos ||
                   lower.find("etw") != std::string::npos;
  if (success && telemetry)
    return Outcome::TELEMETRY_SILENCED;
  if (success)
    return Outcome::EDR_TERMINATED;

  return Outcome::NO_CHANGE;
}

float calculate(bool success, bool detected, const std::string &output,
                float stealthScore) {
  static const std::map<Outcome, float> BASE_REWARD = {
      {Outcome::EDR_TERMINATED, 100.f}, {Outcome::TELEMETRY_SILENCED, 75.f},
      {Outcome::PARTIAL_SUCCESS, 50.f}, {Outcome::NO_CHANGE, 0.f},
      {Outcome::DETECTED_ALERT, -50.f}, {Outcome::ACTION_BLOCKED, -75.f},
      {Outcome::SYSTEM_CRASH, -100.f},
  };

  Outcome outcome = classifyOutcome(success, detected, output);
  float base = BASE_REWARD.at(outcome);

  float stealthBonus = (stealthScore > 0.8f) ? 20.f : 0.f;
  float burnPenalty = detected ? -30.f : 0.f;

  return base + stealthBonus + burnPenalty;
}

} // namespace reward

} // namespace ml
} // namespace edr
