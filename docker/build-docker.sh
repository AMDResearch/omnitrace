#!/usr/bin/env bash

: ${USER:=$(whoami)}
: ${ROCM_VERSIONS:="5.0"}
: ${DISTRO:=ubuntu}
: ${VERSIONS:=20.04}
: ${PYTHON_VERSIONS:="6 7 8 9 10"}
: ${BUILD_CI:=""}
: ${PUSH:=0}

set -e

tolower()
{
    echo "$@" | awk -F '\|~\|' '{print tolower($1)}';
}

toupper()
{
    echo "$@" | awk -F '\|~\|' '{print toupper($1)}';
}

usage()
{
    print_option() { printf "    --%-20s %-24s     %s\n" "${1}" "${2}" "${3}"; }
    echo "Options:"
    print_option "help -h" "" "This message"

    echo ""
    print_default_option() { printf "    --%-20s %-24s     %s (default: %s)\n" "${1}" "${2}" "${3}" "$(tolower ${4})"; }
    print_default_option distro "[ubuntu|opensuse]" "OS distribution" "${DISTRO}"
    print_default_option versions "[VERSION] [VERSION...]" "Ubuntu or OpenSUSE release" "${VERSIONS}"
    print_default_option rocm-versions "[VERSION] [VERSION...]" "ROCm versions" "${ROCM_VERSIONS}"
    print_default_option python-versions "[VERSION] [VERSION...]" "Python 3 minor releases" "${PYTHON_VERSIONS}"
    print_default_option user "[USERNAME]" "DockerHub username" "${USER}"
    #print_default_option lto "[on|off]" "Enable LTO" "${LTO}"
}

send-error()
{
    usage
    echo -e "\nError: ${@}"
    exit 1
}

verbose-run()
{
    echo -e "\n### Executing \"${@}\"... ###\n"
    eval $@
}

reset-last()
{
    last() { send-error "Unsupported argument :: ${1}"; }
}

reset-last

n=0
while [[ $# -gt 0 ]]
do
    case "${1}" in
        -h|--help)
            usage
            exit 0
            ;;
        "--distro")
            shift
            DISTRO=${1}
            last() { DISTRO="${DISTRO} ${1}"; }
            ;;
        "--versions")
            shift
            VERSIONS=${1}
            last() { VERSIONS="${VERSIONS} ${1}"; }
            ;;
        "--rocm-versions")
            shift
            ROCM_VERSIONS=${1}
            last() { ROCM_VERSIONS="${ROCM_VERSIONS} ${1}"; }
            ;;
        "--python-versions")
            shift
            PYTHON_VERSIONS=${1}
            last() { PYTHON_VERSIONS="${PYTHON_VERSIONS} ${1}"; }
            ;;
        --user|-u)
            shift
            USER=${1}
            reset-last
            ;;
        --push)
            PUSH=1
            ;;
        "--*")
            send-error "Unsupported argument at position $((${n} + 1)) :: ${1}"
            ;;
        *)
            last ${1}
            ;;
    esac
    n=$((${n} + 1))
    shift
done

DOCKER_FILE="Dockerfile.${DISTRO}"

if [ -n "${BUILD_CI}" ]; then DOCKER_FILE="${DOCKER_FILE}.ci"; fi
if [ ! -f ${DOCKER_FILE} ]; then cd docker; fi
if [ ! -f ${DOCKER_FILE} ]; then send-error "File \"${DOCKER_FILE}\" not found"; fi

for VERSION in ${VERSIONS}
do
    for i in ${ROCM_VERSIONS}
    do
        CONTAINER=${USER}/omnitrace:release-base-${DISTRO}-${VERSION}-rocm-${i}
        ROCM_MAJOR=$(echo ${i} | sed 's/\./ /g' | awk '{print $1}')
        ROCM_MINOR=$(echo ${i} | sed 's/\./ /g' | awk '{print $2}')
        ROCM_PATCH=$(echo ${i} | sed 's/\./ /g' | awk '{print $3}')
        if [ -n "${ROCM_PATCH}" ]; then
            ROCM_VERSN=$(( (${ROCM_MAJOR}*10000)+(${ROCM_MINOR}*100)+(${ROCM_PATCH}) ))
            ROCM_SEP="."
        else
            ROCM_VERSN=$(( (${ROCM_MAJOR}*10000)+(${ROCM_MINOR}*100) ))
            ROCM_SEP=""
        fi
        if [ "${DISTRO}" = "ubuntu" ]; then
            ROCM_REPO_DIST="ubuntu"
            ROCM_REPO_VERSION=${i}
            case "${i}" in
                4.1* | 4.0*)
                    ROCM_REPO_DIST="xenial"
                    ;;
                *)
                    ;;
            esac
            verbose-run docker build . -f ${DOCKER_FILE} --tag ${CONTAINER} --build-arg DISTRO=${DISTRO} --build-arg VERSION=${VERSION} --build-arg ROCM_REPO_VERSION=${ROCM_REPO_VERSION} --build-arg ROCM_REPO_DIST=${ROCM_REPO_DIST} --build-arg PYTHON_VERSIONS=\"${PYTHON_VERSIONS}\"
        elif [ "${DISTRO}" = "centos" ]; then
            case "${VERSION}" in
                7)
                    RPM_PATH=7.9
                    RPM_TAG=".el7"
                    ;;
                8)
                    RPM_PATH=8.5
                    RPM_TAG=".el7"
                    ;;
                *)
                    send-error "Invalid centos version ${VERSION}. Supported: 7, 8"
            esac
            case "${i}" in
                5.0*)
                    ROCM_RPM=latest/rhel/${RPM_PATH}/amdgpu-install-21.50.50000-1${RPM_TAG}.noarch.rpm
                    ;;
                4.5 | 4.5.2)
                    ROCM_RPM=21.40.2/rhel/${RPM_PATH}/amdgpu-install-21.40.2.40502-1${RPM_TAG}.noarch.rpm
                    ;;
                4.5.1)
                    ROCM_RPM=21.40.1/rhel/${RPM_PATH}/amdgpu-install-21.40.1.40501-1${RPM_TAG}.noarch.rpm
                    ;;
                4.5.0)
                    ROCM_RPM=21.40/rhel/${RPM_PATH}/amdgpu-install-21.40.1.40501-1${RPM_TAG}.noarch.rpm
                    ;;
                *)
                    send-error "Unsupported combination :: ${DISTRO}-${VERSION} + ROCm ${i}"
                    ;;
            esac
            verbose-run docker build . -f ${DOCKER_FILE} --tag ${CONTAINER} --build-arg DISTRO=${DISTRO} --build-arg VERSION=${VERSION} --build-arg AMDGPU_RPM=${ROCM_RPM} --build-arg PYTHON_VERSIONS=\"${PYTHON_VERSIONS}\"
        elif [ "${DISTRO}" = "opensuse" ]; then
            case "${VERSION}" in
                15.*)
                    DISTRO_IMAGE="opensuse/leap"
                    echo "DISTRO_IMAGE: ${DISTRO_IMAGE}"
                    ;;
                *)
                    send-error "Invalid opensuse version ${VERSION}. Supported: 15.x"
                    ;;
            esac
            case "${i}" in
                5.2 | 5.2.*)
                    ROCM_RPM=22.20${ROCM_SEP}${ROCM_PATCH}/sle/${VERSION}/amdgpu-install-22.20.${ROCM_VERSN}-1.noarch.rpm
                    ;;
                5.1 | 5.1.0)
                    ROCM_RPM=22.10/sle/15/amdgpu-install-22.10.50100-1.noarch.rpm
                    ;;
                5.1.*)
                    ROCM_RPM=22.10${ROCM_SEP}${ROCM_PATCH}/sle/15/amdgpu-install-22.10${ROCM_SEP}${ROCM_PATCH}.${ROCM_VERSN}-1.noarch.rpm
                    ;;
                5.0 | 5.0.*)
                    ROCM_RPM=21.50${ROCM_SEP}${ROCM_PATCH}/sle/15/amdgpu-install-21.50${ROCM_SEP}${ROCM_PATCH}.${ROCM_VERSN}-1.noarch.rpm
                    ;;
                4.5 | 4.5.*)
                    ROCM_RPM=21.40${ROCM_SEP}${ROCM_PATCH}/sle/15/amdgpu-install-21.40${ROCM_SEP}${ROCM_PATCH}.${ROCM_VERSN}-1.noarch.rpm
                    ;;
                *)
                    send-error "Unsupported combination :: ${DISTRO}-${VERSION} + ROCm ${i}"
                ;;
            esac
            verbose-run docker build . -f ${DOCKER_FILE} --tag ${CONTAINER} --build-arg DISTRO=${DISTRO_IMAGE} --build-arg VERSION=${VERSION} --build-arg AMDGPU_RPM=${ROCM_RPM} --build-arg PYTHON_VERSIONS=\"${PYTHON_VERSIONS}\"
        fi
        if [ "${PUSH}" -ne 0 ]; then
            docker push ${CONTAINER}
        fi
    done
done
