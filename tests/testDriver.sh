#!/usr/bin/env bash
# set -euo pipefail

EDM4HEP_CMD=${1}
# EDM4HEP follows the same naming scheme as delphes, so the Delphes command name
# can be inferred quite easily
DELPHES_CMD=${EDM4HEP_CMD/_EDM4HEP/}

DELPHES_CARD_IN=${2}
OUTPUT_CONFIG=${3}
OUTPUT_FILE=${4}

DELPHES_CARD=$(pwd)/test_$(basename ${DELPHES_CARD_IN})

# Keep track on whether this has been enabled from the calling site
xtrace_on=$(shopt -qo xtrace && echo "yes")
set +x # Don't need this explicit detailed output for these replacements

# Record the seeds to a file so that we can display them again at script exit if
# something goes wrong for a more convenient way of finding this infomration in
# the vast output of these tests
RANDOM_SEEDS_FILE=$(mktemp $(pwd)/random_seeds.XXXXXXXX)
function seeds_file_cleanup() {
    cat ${RANDOM_SEEDS_FILE}
    rm ${RANDOM_SEEDS_FILE}
}
trap seeds_file_cleanup EXIT

# In order to not have the tests only work with one specific RandomSeed we
# generate a random seed here (unless we override it via the DELPHES_RANDOM_SEED
# env variable) and inject that into the delphes card. This allows us to still
# reproduce the same test run after it has failed because we can see the used
# random seed in the output.
function random_seed_delphes_card() {
    local card_file=${1}
    local out_file=${2}
    if [ "X${DELPHES_RANDOM_SEED}" = "X" ]; then
        local seed=${RANDOM}
    else
        local seed=${DELPHES_RANDOM_SEED}
    fi
    echo "RANDOM SEED FOR DELPHES CARD = ${seed}" | tee -a ${RANDOM_SEEDS_FILE}
    grep "set RandomSeed" ${card_file} > /dev/null 2>&1 && \
        sed "s/set RandomSeed .*/set RandomSeed ${seed}/" ${card_file} > ${out_file}|| \
        sed "/set MaxEvents/a set RandomSeed ${seed}" ${card_file} > ${out_file}
}
random_seed_delphes_card ${DELPHES_CARD_IN} ${DELPHES_CARD}

# Similar to the delphes random seed we generate one for the pythia command, can
# be overridden with PYTHIA_RANDOM_SEED env variable
function random_seed_pythia_cmd() {
    local cmd_file=${1}
    local out_file=${2}
    if [ "X${PYTHIA_RANDOM_SEED}" = "X" ]; then
        local seed=${RANDOM}
    else
        local seed=${PYTHIA_RANDOM_SEED}
    fi
    echo "RANDOM SEED FOR PYTHIA CMD = ${seed}" | tee -a ${RANDOM_SEEDS_FILE}
    grep "Random:seed" ${cmd_file} > /dev/null 2>&1 && \
        sed "s/Random:seed = .*/Random:seed = ${seed}/" ${cmd_file} > ${out_file} || \
        sed "/Main:numberOfEvents/a Random:setSeed = on\nRandom:seed = ${seed}" ${cmd_file} > ${out_file}
}

# restore the xtrace setup
[[ ${xtrace_on} = "yes" ]] && set -x

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
    PYTHIA_CMD=$(pwd)/test_$(basename ${REST_ARGS})
    set +x
    random_seed_pythia_cmd ${REST_ARGS} ${PYTHIA_CMD}
    [[ ${xtrace_on} = "yes" ]] && set -x

    ${EDM4HEP_CMD} ${DELPHES_CARD} ${OUTPUT_CONFIG} ${PYTHIA_CMD} ${OUTPUT_FILE}
    ${DELPHES_CMD} ${DELPHES_CARD} ${PYTHIA_CMD} ${DELPHES_OUTPUT_FILE}
else
    ${EDM4HEP_CMD} ${DELPHES_CARD} ${OUTPUT_CONFIG} ${OUTPUT_FILE} ${REST_ARGS}
    ${DELPHES_CMD} ${DELPHES_CARD} ${DELPHES_OUTPUT_FILE} ${REST_ARGS}
fi

$COMPARE ${OUTPUT_FILE} ${DELPHES_OUTPUT_FILE}
