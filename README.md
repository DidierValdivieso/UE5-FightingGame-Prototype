# Fighting Game Prototype

---

## 🎬 About This Project

This prototype was developed as a **portfolio demonstration** to showcase my experience as a **Gameplay Programmer**. The project was built exclusively for **Unreal Engine 5** and was archived after the studio decided to cancel further development. Even though the full game never reached release, the prototype still demonstrates solid networking, combat, and level design principles.

**My role:**  
> **Role:** Gameplay Programmer  
> • Implemented the character movement and combat systems.  
> • Designed and coded the turn‑based hit‑box system.  
> • Developed client‑side prediction and server reconciliation logic.  
> • Integrated network‑synchronized animations and effects.

---

## 🎨 Visual Highlights

| Description | GIF |
|-------------|-----|
| Clean 1v1 fight – Map A | ![Demo1](Images/gameplayFirstMap.gif) |
| Clean 1v1 fight – Map B | ![Demo2](Images/gameplaySecondMap.gif) |
| Hit‑box debug – Map A | ![Demo3](Images/gameplayFirstMap2.gif) |
| Hit‑box debug – Map B | ![Demo4](Images/gameplaySecondMap2.gif) |

> **Note:** All four GIFs showcase the same core mechanics but on **different arena maps** (two distinct environments) to illustrate level modularity and consistent gameplay across stages.

---

## 🚀 Game Overview

- **Gameplay:** 1v1 and small‑team brawler combat featuring dash, jump, wall‑run, combos, and special abilities.  
- **Arena Design:** Two thematic maps, each with interactive elements that influence combat flow.  
- **Network Architecture:** Authoritative server model with client‑side prediction, hit‑box reconciliation, and lag‑compensation.  
- **Technology:** Unreal Engine 5 (C++ + Blueprints) with a custom UDP protocol and TCP fallback.

---

## 🧩 Project Structure

```
FightingGamePrototype/
├── Config/               # Game configuration files
├── Content/               # Assets: textures, sounds, animations, etc.
├── Source/
│   ├── FightingGamePrototype/          # Main module
│   └── FightingGamePrototypeEditor/  # Editor tools
├── Images/                # GIFs used in this README
├── .gitignore
└── README.md
