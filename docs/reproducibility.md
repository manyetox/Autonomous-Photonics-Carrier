# Reproducibility checklist

A build or experiment is only reproducible if someone else can determine exactly what was used.

For each published result, record:

- Platform V1 or Platform V2;
- every PCB board ID/revision;
- exact fabrication package or commit;
- component substitutions;
- final mechanical CAD/revision;
- wheel/motor/magnet geometry;
- battery and supply configuration;
- firmware commit/tag;
- host-software commit/tag;
- calibration files;
- raw data;
- processing script/notebook version;
- environmental/tabletop conditions;
- ground-truth method and calibration;
- experiment protocol and exclusion criteria.

## Recommended dataset layout

```text
data/
└── YYYY-MM-DD_experiment-name/
    ├── README.md
    ├── metadata.yaml
    ├── raw/
    ├── calibration/
    ├── processed/
    └── figures/
```

Do not commit large raw datasets blindly to normal Git history. Curated datasets can later be archived separately and referenced from the repository.
