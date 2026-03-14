# Fighting Game Prototype

A fast-paced multiplayer brawler game built with Unreal Engine 5. Experience intense combat in a world where ancient powers clash.

## 🔄 Demo in action

![GameplayDemo](Images/gameplayFirstMap.gif)

## 🎮 Game Overview

Fighting Game is an action-packed multiplayer brawler game featuring:
- Intense 1v1 and team-based combat
- Unique character abilities based on ancient powers
- Dynamic arena environments
- Real-time multiplayer gameplay with netcode support

## 🔧 Features

### Core Gameplay
- **Character Movement System**: Smooth character controls with dash, jump, and wall-run mechanics
- **Combat System**: Combo-based attacks, special abilities, and environmental interactions
- **Power-up System**: Temporary buffs and status effects during matches
- **Arena Design**: Multiple themed environments with interactive elements

### Netcode Implementation
- **Client-Server Architecture**: Reliable server-authoritative gameplay
- **Prediction & Correction**: Smooth client-side prediction with server reconciliation
- **Lag Compensation**: Bullet time and hit registration systems
- **Network Synchronization**: Character states, animations, and combat events
- **Matchmaking System**: Real-time lobby creation and connection handling

### Technical Features
- **Unreal Engine 5**: Utilizing the latest engine features for optimal performance
- **Blueprints & C++**: Hybrid development approach for flexibility
- **Asset Management**: Modular content system with proper asset organization
- **UI/UX Design**: Intuitive interfaces for gameplay and menus

## 📁 Project Structure

```
FightingGamePrototype/
├── Config/              # Game configuration files
├── Content/             # Game assets, textures, sounds, etc.
├── Source/              # C++ source code
│   ├── FightingGamePrototype/  # Main game module
│   └── FightingGamePrototypeEditor/  # Editor extensions
├── .gitignore           # Git ignore patterns
└── README.md            # This file
```

## 🚀 Future Development

### Planned Features
- Cross-platform multiplayer support
- Character customization system
- Ranked matchmaking system
- Tournament mode
- Additional arena maps
- Voice chat integration

### Performance Improvements
- Optimized network packet compression
- Enhanced client-side prediction algorithms
- Improved server load balancing

## 📊 Technical Specifications

### Network Protocol
- **Protocol**: Custom UDP-based protocol with TCP fallback
- **Latency**: < 50ms average in optimal conditions
- **Bandwidth**: ~100KB/s per player during active gameplay

## 🎯 Game Design Philosophy

Fighting Game focuses on:
- **Accessibility**: Simple controls with deep strategic depth
- **Competitive Balance**: Regular balancing updates based on community feedback
- **Performance**: Smooth gameplay at 60+ FPS
- **Scalability**: Modular design for easy feature additions
