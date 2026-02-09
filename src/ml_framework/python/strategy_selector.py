"""
Strategy Selector - Reinforcement Learning Agent for EDR Termination
Author: Karthik
Description: DQN agent that selects optimal attack techniques based on system state
"""

import torch
import torch.nn as nn
import torch.optim as optim
import numpy as np
from collections import deque
import random
import json
from pathlib import Path


class DQNNetwork(nn.Module):
    """
    Deep Q-Network: Maps state -> Q-values for each action
    """
    def __init__(self, state_size, action_size, hidden_size=128):
        super(DQNNetwork, self).__init__()
        self.network = nn.Sequential(
            nn.Linear(state_size, hidden_size),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(hidden_size, hidden_size // 2),
            nn.ReLU(),
            nn.Dropout(0.2),
            nn.Linear(hidden_size // 2, action_size)
        )
    
    def forward(self, x):
        return self.network(x)


class DQNAgent:
    """
    Deep Q-Network Agent for attack strategy selection
    
    State Space (30 features):
        - EDR info: product, version, running, driver loaded
        - System: Windows version, integrity level, PPL, DSE, Secure Boot
        - History: previous actions, results, failure count
    
    Action Space (8 actions):
        0: BYOVD_RTCore
        1: BYOVD_DBUtil
        2: PPL_Bypass
        3: Handle_Duplication
        4: Minifilter_Unload
        5: Callback_Removal
        6: Direct_Syscall
        7: Wait_Observe
    
    Reward Function:
        +100: EDR terminated
        +75:  Telemetry silenced
        +50:  Partial success
        0:    No change
        -50:  Detected
        -75:  Blocked
        -100: System crash
    """
    
    def __init__(self, state_size=30, action_size=8, learning_rate=0.001):
        self.state_size = state_size
        self.action_size = action_size
        
        # Hyperparameters
        self.gamma = 0.95           # Discount factor
        self.epsilon = 1.0          # Exploration rate
        self.epsilon_min = 0.01     # Minimum exploration
        self.epsilon_decay = 0.995  # Exploration decay
        self.learning_rate = learning_rate
        self.batch_size = 32
        
        # Replay memory
        self.memory = deque(maxlen=10000)
        
        # Q-Networks
        self.policy_net = DQNNetwork(state_size, action_size)
        self.target_net = DQNNetwork(state_size, action_size)
        self.target_net.load_state_dict(self.policy_net.state_dict())
        self.target_net.eval()  # Target network never trains directly
        
        # Optimizer
        self.optimizer = optim.Adam(self.policy_net.parameters(), lr=learning_rate)
        
        # Action mapping
        self.action_names = [
            "BYOVD_RTCore",
            "BYOVD_DBUtil", 
            "PPL_Bypass",
            "Handle_Duplication",
            "Minifilter_Unload",
            "Callback_Removal",
            "Direct_Syscall",
            "Wait_Observe"
        ]
    
    def select_action(self, state, valid_actions=None):
        """
        Select action using epsilon-greedy policy
        
        Args:
            state: Current system state (numpy array or list)
            valid_actions: Optional list of valid action indices
        
        Returns:
            action_id: Integer action index
        """
        # Exploration: random action
        if np.random.rand() <= self.epsilon:
            if valid_actions:
                return random.choice(valid_actions)
            return random.randint(0, self.action_size - 1)
        
        # Exploitation: best predicted action
        with torch.no_grad():
            state_tensor = torch.FloatTensor(state).unsqueeze(0)
            q_values = self.policy_net(state_tensor)
            
            if valid_actions:
                # Mask invalid actions
                q_values_np = q_values.numpy()[0]
                valid_q = {a: q_values_np[a] for a in valid_actions}
                return max(valid_q, key=valid_q.get)
            
            return torch.argmax(q_values).item()
    
    def remember(self, state, action, reward, next_state, done):
        """
        Store experience in replay buffer
        
        Args:
            state: Current state
            action: Action taken
            reward: Reward received
            next_state: Resulting state
            done: Episode terminated?
        """
        self.memory.append((state, action, reward, next_state, done))
    
    def replay(self, batch_size=None):
        """
        Train on random batch from memory (Experience Replay)
        
        Args:
            batch_size: Number of samples to train on
        """
        if batch_size is None:
            batch_size = self.batch_size
        
        if len(self.memory) < batch_size:
            return  # Not enough samples yet
        
        # Sample random minibatch
        minibatch = random.sample(self.memory, batch_size)
        
        # Prepare tensors
        states = torch.FloatTensor([e[0] for e in minibatch])
        actions = torch.LongTensor([e[1] for e in minibatch])
        rewards = torch.FloatTensor([e[2] for e in minibatch])
        next_states = torch.FloatTensor([e[3] for e in minibatch])
        dones = torch.FloatTensor([e[4] for e in minibatch])
        
        # Current Q-values (from policy network)
        current_q = self.policy_net(states).gather(1, actions.unsqueeze(1))
        
        # Target Q-values (from target network)
        with torch.no_grad():
            max_next_q = self.target_net(next_states).max(1)[0]
            target_q = rewards + (1 - dones) * self.gamma * max_next_q
        
        # Compute loss
        loss = nn.MSELoss()(current_q.squeeze(), target_q)
        
        # Backpropagation
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        
        # Decay exploration rate
        if self.epsilon > self.epsilon_min:
            self.epsilon *= self.epsilon_decay
        
        return loss.item()
    
    def update_target_network(self):
        """
        Copy weights from policy network to target network
        Call this every N episodes (e.g., 10)
        """
        self.target_net.load_state_dict(self.policy_net.state_dict())
    
    def save(self, filepath):
        """Save model checkpoint"""
        torch.save({
            'policy_net_state': self.policy_net.state_dict(),
            'target_net_state': self.target_net.state_dict(),
            'optimizer_state': self.optimizer.state_dict(),
            'epsilon': self.epsilon,
            'memory': list(self.memory)[-1000:]  # Save last 1000 experiences
        }, filepath)
        print(f"✓ Model saved to {filepath}")
    
    def load(self, filepath):
        """Load model checkpoint"""
        checkpoint = torch.load(filepath)
        self.policy_net.load_state_dict(checkpoint['policy_net_state'])
        self.target_net.load_state_dict(checkpoint['target_net_state'])
        self.optimizer.load_state_dict(checkpoint['optimizer_state'])
        self.epsilon = checkpoint['epsilon']
        if 'memory' in checkpoint:
            self.memory = deque(checkpoint['memory'], maxlen=10000)
        print(f"✓ Model loaded from {filepath}")
    
    def get_action_name(self, action_id):
        """Convert action ID to technique name"""
        return self.action_names[action_id]


# ============================================================================
# TESTING / DEMO CODE
# ============================================================================

def test_agent():
    """
    Basic test to verify agent works
    """
    print("=" * 60)
    print("Testing DQN Agent")
    print("=" * 60)
    
    agent = DQNAgent(state_size=30, action_size=8)
    
    # Dummy state (30 features, all zeros)
    state = np.zeros(30)
    state[0] = 1.0  # EDR running
    state[5] = 1.0  # PPL enabled
    
    print("\n1. Testing action selection...")
    action = agent.select_action(state)
    print(f"   Selected action: {action} ({agent.get_action_name(action)})")
    print(f"   Epsilon (exploration rate): {agent.epsilon:.3f}")
    
    print("\n2. Testing memory storage...")
    next_state = state.copy()
    next_state[0] = 0.0  # EDR terminated!
    agent.remember(state, action, reward=100, next_state=next_state, done=True)
    print(f"   Memory size: {len(agent.memory)}")
    
    print("\n3. Collecting more experiences...")
    for i in range(100):
        s = np.random.rand(30)
        a = random.randint(0, 7)
        r = random.choice([100, -50, 0, 50])
        s_next = np.random.rand(30)
        agent.remember(s, a, r, s_next, done=(r == 100))
    print(f"   Memory size: {len(agent.memory)}")
    
    print("\n4. Testing training (replay)...")
    loss = agent.replay(batch_size=32)
    print(f"   Loss: {loss:.4f}")
    print(f"   Epsilon after training: {agent.epsilon:.3f}")
    
    print("\n5. Testing save/load...")
    save_path = Path(__file__).parent.parent / "models" / "test_agent.pth"
    save_path.parent.mkdir(exist_ok=True)
    agent.save(save_path)
    
    new_agent = DQNAgent(state_size=30, action_size=8)
    new_agent.load(save_path)
    print(f"   Loaded epsilon: {new_agent.epsilon:.3f}")
    
    print("\n✓ All tests passed!")
    print("=" * 60)


if __name__ == "__main__":
    test_agent()
