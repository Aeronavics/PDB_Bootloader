pipeline {
    agent any

    options {
        timestamps()
        disableConcurrentBuilds()
        buildDiscarder(logRotator(numToKeepStr: '20'))
    }

    environment {
        PROJECT     = 'PDB_Bootloader'
        BRANCH      = "${env.BRANCH_NAME}"
        FTP_OUT     = "/home/ftp/ftpshare/build_outputs/PDBBootloader/${env.BRANCH_NAME}"
        STATE_DIR   = "/home/ftp/jenkins/PDBBootloader/${env.BRANCH_NAME}"
        RELEASE_DIR = '/home/releaseLibrary/AC-16/Embedded/Bootloaders'
        CUBEIDE     = '/opt/st/stm32cubeide_2.2.0/stm32cubeide'
        // Unique per build. An aborted run skips post{cleanup}, leaving a
        // locked .metadata in place; a fixed path would then fail every later
        // build with "Workspace already in use!" until /tmp was cleared.
        IDE_WS      = "/tmp/stm32cubeide-headless-ws-${env.BUILD_TAG.replaceAll('[^A-Za-z0-9._-]', '_')}"
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
                // The submodules are https://github.com/... so they need the
                // credential explicitly; checkout scm does not pass it on.
                withCredentials([usernamePassword(
                        credentialsId: 'Aeronavics_Jenkins',
                        usernameVariable: 'GIT_USER',
                        passwordVariable: 'GIT_TOKEN')]) {
                    sh '''
                        set -eu
                        # sync picks up .gitmodules URL changes (ssh -> https)
                        # that a previously initialised workspace still caches.
                        git submodule sync --recursive

                        # -c is exported as GIT_CONFIG_PARAMETERS, so the helper
                        # is inherited by the nested submodule fetches too. The
                        # empty first value clears any helper already configured.
                        # --force re-checkouts a submodule even when the index
                        # already matches, as it does for a workspace left with
                        # empty submodule directories by a failed run.
                        git -c credential.helper= \
                            -c credential.helper='!f() { echo "username=$GIT_USER"; echo "password=$GIT_TOKEN"; }; f' \
                            submodule update --init --recursive --force

                        git submodule status --recursive
                    '''
                }
            }
        }

        stage('Generate sources') {
            steps {
                sh '''
                    set -eu
                    cd Modules
                    for f in dronecan_dsdlc/dronecan_dsdlc.py \
                             mavlink/message_definitions/v1.0/all.xml; do
                        [ -e "$f" ] || { echo "Missing $f - submodule not checked out" >&2; exit 1; }
                    done

                    python3 dronecan_dsdlc/dronecan_dsdlc.py -O dsdl_generated \
                        DSDL/uavcan DSDL/dronecan DSDL/com DSDL/ardupilot
                    mkdir -p mavlink_generated/mavlink
                    PYTHONPATH="$PWD/mavlink" python3 -c "
from pymavlink.generator import mavgen
opts = mavgen.Opts(output='mavlink_generated/mavlink', wire_protocol='2.0', language='C', validate=False)
mavgen.mavgen(opts, ['mavlink/message_definitions/v1.0/all.xml'])"
                '''
            }
        }

        stage('Build') {
            steps {
                sh '''#!/bin/bash
                    set -euo pipefail
                    # Debug/ is gitignored build output. An empty Debug/ makes
                    # CDT run "make clean" against a directory with no makefile,
                    # which fails the build before makefiles are generated, so
                    # remove it entirely and let the managed build recreate it.
                    rm -rf Debug

                    set +e
                    "$CUBEIDE" \
                        --launcher.suppressErrors \
                        -nosplash \
                        -application org.eclipse.cdt.managedbuilder.core.headlessbuild \
                        -data "$IDE_WS" \
                        -import "./" \
                        -build "$PROJECT/Debug" 2>&1 | tee cubeide-build.log
                    ide_status=${PIPESTATUS[0]}
                    set -e

                    # Every build exits 1: CDT's ld error parser counts the
                    # "ld: ...: in function `_gettimeofday_r':" line as an error,
                    # though the severity on the following line is warning. That
                    # message comes from newlib's time() with no _gettimeofday in
                    # Core/Src/syscalls.c, and is harmless. Print real compiler
                    # diagnostics so a genuine failure is not lost behind it.
                    if [ "$ide_status" -ne 0 ]; then
                        echo "===== STM32CubeIDE exited $ide_status; diagnostics ====="
                        grep -nE 'error:|Error [0-9]+|fatal:|No such file' \
                            cubeide-build.log || echo '(no compiler or make errors)'
                        echo "===== end ====="
                    fi

                    # Judge the build on the deliverable: a compile or link
                    # failure leaves no binary, and a failed post-build rename
                    # leaves no bootloader_pdb* file, so this catches both.
                    ls -l Debug/bootloader_pdb*.bin
                '''
            }
        }

        stage('Deploy') {
            steps {
                sh '''
                    set -eu
                    current_version=$(git rev-parse HEAD)
                    mkdir -p "$FTP_OUT"

                    # Drop any stale artefacts for this same commit.
                    rm -f "$FTP_OUT/$current_version.bin" \
                          "$FTP_OUT/$current_version.md5" \
                          "$FTP_OUT/$current_version.txt"

                    # Publish to the FTP server.
                    cp Debug/bootloader_pdb* "$FTP_OUT/$current_version.bin"
                    git log -1 --pretty=%B > "$FTP_OUT/$current_version.txt"

                    # Refresh the release library.
                    rm -f "$RELEASE_DIR"/bootloader_pdb*
                    cp Debug/bootloader_pdb* "$RELEASE_DIR/"

                    # Point latest.* at this build.
                    cd "$FTP_OUT"
                    rm -f latest.bin latest.md5 latest.txt
                    cp "$current_version.bin" latest.bin
                    md5sum latest.bin | cut -c -32 > "$current_version.md5"
                    cp "$current_version.md5" latest.md5
                    cp "$current_version.txt" latest.txt
                '''

                script {
                    // The binary carries the version and commit in its name,
                    // and nexusArtifactUploader takes a literal path, so resolve
                    // the glob here rather than hardcoding a filename.
                    String bootloader = sh(
                        returnStdout: true,
                        script: 'ls Debug/bootloader_pdb*.bin | head -1'
                    ).trim()
                    echo "Uploading ${bootloader} to Nexus"

                    nexusArtifactUploader(
                        nexusVersion: 'nexus3',
                        protocol: 'https',
                        nexusUrl: 'nexus.aeronavics.com',
                        credentialsId: 'JenkinsAdmin',
                        repository: 'release_library',
                        groupId: 'bootloader',
                        version: 'latest',
                        artifacts: [[
                            artifactId: 'pdb',
                            file: bootloader,
                            type: 'bin',
                            classifier: ''
                        ]]
                    )
                }

                // Recorded last, once every publish has succeeded, so that a
                // failed deploy or upload is retried on the next build.
                sh 'echo "$(git rev-parse HEAD)" > "$STATE_DIR/build_version"'

                archiveArtifacts artifacts: 'Debug/bootloader_pdb*',
                                 fingerprint: true,
                                 allowEmptyArchive: true
            }
        }
    }

    post {
        cleanup {
            sh 'rm -rf "$IDE_WS" || true'
        }
    }
}
