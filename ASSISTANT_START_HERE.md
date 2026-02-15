# Assistant Start Here (Read First)

This project is a vintage radio retrofit using Arduino IDE.

## Where to Reference the Project

### Public Mirror (Use this for browsing in chat)
https://github.com/jcobris/Vintage-Radio-1-public

> The assistant should reference files via the public mirror link in future conversations.

### Private Repo (Source of truth)
`jcobris/Vintage-Radio-1` (private)
> Development happens here; it mirrors automatically to the public repo via GitHub Actions.

## Mirror Details (for context)
- Private repo workflow: `.github/workflows/mirror-to-public.yml`
- Uses private repo secret: `MIRROR_PAT`
- Public mirror updates automatically after pushes to the private repo.

## Working Style
- One step at a time.
- Avoid big rewrites unless explicitly requested.
- Arduino Nano first; ESP32 only if necessary due to resources.

## Read These Files First
- `PROJECT_STATE.md`
- `docs/REQUIREMENTS.md`
- `docs/ARCHITECTURE.md`
- `docs/PINOUT.md`
- `docs/README.md`

## Key Code File
- Current working sketch: `sketch/Radio_Main.ino`

## Typical Session Goal
- Help merge and refine functionality safely, incrementally, with clear acceptance checks.
