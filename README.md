# RROML

---

> [!CAUTION]
> RROML and mods may cause unexpected bugs or issues in multiplayer. Feel free to report said issues in the RROML GitHub repository Issues section. Please be sure to back up your saves before modding in multiplayer.

---

**Legend:**

This is what all shorthands mean or stand for here. They will be used throughout every mention of RROML and RRO modding regardless of repository or community.

RROML -- Railroads Online Mod Loader

RO / RRO -- Railroads Online

QoL -- Quality of Life

ROMC / RROMC -- Railroads Online Modding Community

Repo -- Repository

PR(s) -- Pull Request(s)

---

**Short Description:**

RROML is a mod loader for the game Railroads Online! It makes it so that the community can make mods for the game to improve QoL, add new things, and much more!

---

**Information:**
- RROML is coded in C++ & C#
- Mods are coded in C# and **Lua** (since v1.2.0, via MoonSharp Lua 5.2)
- Compiling uses Batchfiles (.bat) and C# / C++ compilers, or any other means of compilation of C# & C++ code
- Lua mods do **not** require compilation – just drop a `main.lua` (+ `mod.json` with `EntryLua`) into `Mods/YourMod/` or a single `Mods/YourMod.lua`

---

**Installation:**

Please see the [RRO Modding Wiki for installation](https://github.com/KerbalMissile/Railroads-Online-Modding-Wiki/wiki)

---

My (KerbalMissile)'s mods can be found here:

https://github.com/KerbalMissile/RRO-Mods-KM

---

**Roadmap:**

| Phase | Plan | Completion Status|
|------|--------|--------|
| Phase: 0 | Basic Game Detection + Mod Loader Boot | Completed |
| Phase: 1 | Code Injection | Completed With Possible Updates and Fixes |
| Phase: 1 | .pak Inspection | Completed via UEBridge mod |
| Phase: 1 | .exe Inspection | Mostly Completed via UEBridge mod |
| Phase: 2 | .pak Custom Models & Sounds | In Development |
| Phase: 2 | Calling Textures From .pak's | Planned + Partially In Development |
| Phase: 3 | .pak Custom Textures | Planned |
| Phase: 3 | .pak Custom + Pre-Existing Textures | Planned |
| Phase: 4 | QoL Improvements & Bug Fixes | Planned |
| Phase: 5 | .pak Texture Changes | Planned |
| Phase: 6 | All Main Features Completed & Minor Bug Fixes and Compatibility Support | Planned |

---

**Buildings From Source:**

This is for if you want to contribute code to RROML but need to test first.

Steps:
1. Fork the repo and clone your fork using `git clone https://github.com/USERNAME/RROML.git`, note this command only clones it locally and doesn't fork it for you.
2. Make your changes to your local code.
3. Open up a terminal at the path to the RROML folder, could be something like D:\RROML
4. From there edit this command to point to your Railroads Online folder: `.\tools\install.bat "D:\SteamLibrary\steamapps\common\Railroads Online"`
5. Now all you have to do is run your edited command from the RROML folder.
6. Then you are done and can now test.

---

Links:

Please look at the wiki first before asking for help in the Discord.

[RRO Modding Discord](https://discord.gg/4UZ3CQca4R)

[RRO Modding Wiki](https://github.com/KerbalMissile/Railroads-Online-Modding-Wiki/wiki)

---

**Licensing:**

RROML is licensed under the [KPL-v1.1 license](LICENSE.md).
