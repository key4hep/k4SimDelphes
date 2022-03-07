## Docker Image for k4SimDelphes 

This image contains:

1. ROOT (6.24/06)
2. LCIO (v02-16-01)
3. Delphes (3.5.0)
4. Podio
5. EDM4HEP
6. k4SimDelphes (with support for reading STDHEP files in standalone mode only)


if you want to build the image yourself,
```bash
docker build .
```

Otherwise, the image is here:
```bash
docker pull ilcsoft/k4simdelphes:latest
```