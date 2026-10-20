#!/usr/bin/env python3
"""Compile le rapport depuis son dossier pour résoudre tableaux et figures."""
from pathlib import Path
import subprocess

rapport = Path(__file__).resolve().parent / 'rapport'
for _ in range(2):
    subprocess.run(['pdflatex', '-interaction=nonstopmode', '-halt-on-error',
                    'rapport.tex'], cwd=rapport, check=True)
print(f'PDF créé : {rapport / "rapport.pdf"}')
