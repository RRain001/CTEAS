# Environment Setup (CALPUFF tutorial)

## Why missing modules happens

Your old environments at:

- `/home/ai/code/.venv`
- `/home/ai/code/.venv_isd`

were incomplete (no `pip`, no `ensurepip`), so dependency installation was not reliable.

## Standard workflow for this project

From your tutorial directory (for example `/home/ai/tutorial`):

```bash
./bootstrap_venv.sh
```

Then run with the unified CLI:

```bash
.venv/bin/python -m engine.orchestrator.cli run --case test --stage all
# or
./calpuff-cli run --case test --stage all
```

Preprocess is now non-interactive and case-driven. Configure these fields in
`project/cases/<case>/case.yaml` before running:

- `preprocess.grid`
- `preprocess.research_time`
- `preprocess.pipeline`
- `preprocess.upperair`
- `preprocess.surface`

Local GUI (desktop):

```bash
./calpuff-cli desktopgui
```

## Migration to another computer

1. Copy the whole `tutorial` folder.
2. On the new machine, install `python3` (and `uv` recommended).
3. In the copied folder, run:

```bash
./bootstrap_venv.sh
```

Do not copy `.venv` across machines. Recreate it from `requirements.txt`.
