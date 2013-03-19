/* Copyright (c) 2012, Rice University

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1.  Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.  Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.
3.  Neither the name of Intel Corporation
     nor the names of its contributors may be used to endorse or
     promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <assert.h>
#include "ocr-communication.h"
#include "ocr-runtime.h"

ocr_communicator_t * newCommunicator(ocr_communicator_kind communicatorType) {
    switch(communicatorType) {
    case OCR_HC_MPI:
        return ocr_hc_mpi_communicator_constructor(communicatorType);
    default:
        assert(false && "Unrecognized communicator kind");
        break;
    }

    return NULL;
}

ocr_communicator_t * ocrGetCommunicator(ocr_communicator_kind communicatorType) {
    int i;
    ocrGuid_t workerGuid = ocr_get_current_worker_guid();
    ocr_worker_t * worker = (ocr_worker_t*)deguidify(workerGuid);
    ocr_policy_domain_t * policy = (ocr_policy_domain_t *)deguidify(worker->getCurrentPolicyDomain(worker));
    for (i = 0; i < policy->nb_communicators; i++)
        if (policy->communicators[i]->kind == communicatorType)
            return policy->communicators[i];
    return NULL;
}

