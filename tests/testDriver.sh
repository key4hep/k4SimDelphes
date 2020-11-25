#!/usr/bin/env bash
# set -euo pipefail

EDM4HEP_CMD=${1}
# EDM4HEP follows the same naming scheme as delphes, so the Delphes command name
# can be inferred quite easily
DELPHES_CMD=${EDM4HEP_CMD/_EDM4HEP/}

DELPHES_CARD=${2}
OUTPUT_CONFIG=${3}
OUTPUT_FILE=${4}
# Derive the outputfile name for delphes from the output file name of the
# edm4hep run
DELPHES_OUTPUT_FILE=${OUTPUT_FILE/.root/_delphes.root}
shift 4
REST_ARGS=${@}

# Delphes will not overwrite existing files
[ -f ${DELPHES_OUTPUT_FILE} ] && rm ${DELPHES_OUTPUT_FILE}

# Pythia needs special treatment here because the clargs do not follow the same
# convention as for the others
if [ ${DELPHES_CMD/Delphes/} = 'Pythia8' ]; then
    ${EDM4HEP_CMD} ${DELPHES_CARD} ${OUTPUT_CONFIG} ${REST_ARGS} ${OUTPUT_FILE}
    ${DELPHES_CMD} ${DELPHES_CARD} ${REST_ARGS} ${DELPHES_OUTPUT_FILE}
else
    ${EDM4HEP_CMD} ${DELPHES_CARD} ${OUTPUT_CONFIG} ${OUTPUT_FILE} ${REST_ARGS}
    ${DELPHES_CMD} ${DELPHES_CARD} ${DELPHES_OUTPUT_FILE} ${REST_ARGS}
fi

$COMPARE ${OUTPUT_FILE} ${DELPHES_OUTPUT_FILE}
