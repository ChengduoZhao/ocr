#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ocr-runtime.h"
#include "ocr-config.h"
#include "ocr-guid.h"

// Fwd declarations
static ocr_model_policy_t * createThorRootModelPolicy ( );
static ocr_model_policy_t * createThorL3ModelPolicies ( size_t n_L3s );
static ocr_model_policy_t * createThorL2ModelPolicies ( size_t n_L2s );
static ocr_model_policy_t * createThorL1ModelPolicies ( size_t n_L1s );
static ocr_model_policy_t * createThorWorkerModelPolicies ( size_t n_workers );
static ocr_model_policy_t * createThorMasteredWorkerModelPolicies ( );

void ocrModelInitThor(char * mdFile) {
    size_t n_L3s = 2;
    size_t n_L2s_per_L3 = 8;
    size_t n_L1s_per_L2 = 1;
    size_t n_workers_per_L1 = 1;

    size_t n_L2s = n_L2s_per_L3 * n_L3s; // 16
    size_t n_L1s = n_L1s_per_L2 * n_L2s; // 16
    size_t n_workers = n_workers_per_L1 * n_L1s; //16

    ocr_model_policy_t * root_policy_model = createThorRootModelPolicy ( );
    ocr_model_policy_t * l3_policy_model = createThorL3ModelPolicies ( n_L3s );
    ocr_model_policy_t * l2_policy_model = createThorL2ModelPolicies ( n_L2s );
    ocr_model_policy_t * l1_policy_model = createThorL1ModelPolicies ( n_L1s );
    ocr_model_policy_t * mastered_worker_policy_model = createThorMasteredWorkerModelPolicies ( );
    ocr_model_policy_t * worker_policy_model = createThorWorkerModelPolicies ( n_workers - 1 );

    ocr_policy_domain_t ** thor_root_policy_domains = instantiateModel(root_policy_model);
    ocr_policy_domain_t ** thor_l3_policy_domains = instantiateModel(l3_policy_model);
    ocr_policy_domain_t ** thor_l2_policy_domains = instantiateModel(l2_policy_model);
    ocr_policy_domain_t ** thor_l1_policy_domains = instantiateModel(l1_policy_model);
    ocr_policy_domain_t ** thor_mastered_worker_policy_domains = instantiateModel(mastered_worker_policy_model);
    ocr_policy_domain_t ** thor_worker_policy_domains = instantiateModel(worker_policy_model);

    n_root_policy_nodes = 1;
    root_policies = (ocr_policy_domain_t**) malloc(sizeof(ocr_policy_domain_t*));
    root_policies[0] = thor_root_policy_domains[0];

    size_t breadthFirstLabel = 0;

    thor_root_policy_domains[0]->n_successors = n_L3s;
    thor_root_policy_domains[0]->successors = thor_l3_policy_domains;
    thor_root_policy_domains[0]->n_predecessors = 0;
    thor_root_policy_domains[0]->predecessors = NULL;
    thor_root_policy_domains[0]->id = breadthFirstLabel++; 

    size_t idx = 0;
    for ( idx = 0; idx < n_L3s; ++idx ) {
        ocr_policy_domain_t *curr = thor_l3_policy_domains[idx];
        curr->id = breadthFirstLabel++; 

        curr->n_successors = n_L2s_per_L3;
        curr->successors = &(thor_l2_policy_domains[idx*n_L2s_per_L3]);
        curr->n_predecessors = 1;
        curr->predecessors = thor_root_policy_domains;
    }

    for ( idx = 0; idx < n_L2s; ++idx ) {
        ocr_policy_domain_t *curr = thor_l2_policy_domains[idx];
        curr->id = breadthFirstLabel++; 

        curr->n_successors = n_L1s_per_L2;
        curr->successors = &(thor_l1_policy_domains[idx*n_L1s_per_L2]);
        curr->n_predecessors = 1;
        curr->predecessors = &(thor_l3_policy_domains[idx/n_L2s_per_L3]);
    }

    // idx = 0 condition for ( idx = 0; idx < n_L1s; ++idx )
    {
        ocr_policy_domain_t **nasty_successor_buffering = (ocr_policy_domain_t **)malloc(n_workers_per_L1*sizeof(ocr_policy_domain_t *));
        nasty_successor_buffering[0] = thor_mastered_worker_policy_domains[0];
        for ( idx = 1; idx < n_workers_per_L1; ++idx ) {
            nasty_successor_buffering[idx] = thor_worker_policy_domains[idx-1];
        }
        idx = 0;
        ocr_policy_domain_t *curr = thor_l1_policy_domains[idx];
        curr->id = breadthFirstLabel++; 

        curr->n_successors = n_workers_per_L1;
        curr->successors = nasty_successor_buffering;
        curr->n_predecessors = 1;
        curr->predecessors = &(thor_l2_policy_domains[idx/n_L1s_per_L2]);
    }

    for ( idx = 1; idx < n_L1s; ++idx ) {
        ocr_policy_domain_t *curr = thor_l1_policy_domains[idx];
        curr->id = breadthFirstLabel++; 

        curr->n_successors = n_workers_per_L1;
        curr->successors = &(thor_worker_policy_domains[idx*n_workers_per_L1 - 1]);
        curr->n_predecessors = 1;
        curr->predecessors = &(thor_l2_policy_domains[idx/n_L1s_per_L2]);
    }

    // idx = 0 condition for ( idx = 1; idx < n_workers; ++idx ) 
    {
        idx = 0;
        ocr_policy_domain_t *curr = thor_mastered_worker_policy_domains[idx];
        curr->id = breadthFirstLabel++; 
        curr->n_successors = 0;
        curr->successors = NULL;
        curr->n_predecessors = 1;
        curr->predecessors = &(thor_l1_policy_domains[idx/n_workers_per_L1]);
    }

    for ( idx = 1; idx < n_workers; ++idx ) {
        ocr_policy_domain_t *curr = thor_worker_policy_domains[idx-1];
        curr->id = breadthFirstLabel++; 

        curr->n_successors = 0;
        curr->successors = NULL;
        curr->n_predecessors = 1;
        curr->predecessors = &(thor_l1_policy_domains[idx/n_workers_per_L1]);
    }

    // does not do anything as these are mere 'empty' places
    thor_root_policy_domains[0]->start(thor_root_policy_domains[0]);
    for ( idx = 0; idx < n_L3s; ++idx ) {
        thor_l3_policy_domains[idx]->start(thor_l3_policy_domains[idx]);
    }
    for ( idx = 0; idx < n_L2s; ++idx ) {
        thor_l2_policy_domains[idx]->start(thor_l2_policy_domains[idx]);
    }
    for ( idx = 0; idx < n_L1s; ++idx ) {
        thor_l1_policy_domains[idx]->start(thor_l1_policy_domains[idx]);
    }


    thor_mastered_worker_policy_domains[0]->start(thor_mastered_worker_policy_domains[0]);
    for ( idx = 1; idx < n_workers; ++idx ) {
        thor_worker_policy_domains[idx-1]->start(thor_worker_policy_domains[idx-1]);
    }

    master_worker = thor_mastered_worker_policy_domains[0]->workers[0];
}

static ocr_model_policy_t * createEmptyModelPolicyHelper ( size_t nPlaces ) {
    ocr_model_policy_t * policyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));
    policyModel->model.kind = OCR_PLACE_POLICY;
    policyModel->model.nb_instances = nPlaces;
    policyModel->model.per_type_configuration = NULL;
    policyModel->model.per_instance_configuration = NULL;
    policyModel->nb_scheduler_types = 0;
    policyModel->nb_worker_types    = 0;
    policyModel->nb_comp_target_types  = 0;
    policyModel->nb_workpile_types  = 0;
    policyModel->numMemTypes = 0;
    policyModel->numAllocTypes = 0;
    policyModel->nb_mappings = 0;

    policyModel->schedulers = NULL;
    policyModel->workers = NULL;
    policyModel->compTargets = NULL;
    policyModel->workpiles = NULL;
    policyModel->memories = NULL;
    policyModel->allocators = NULL;
    policyModel->mappings = NULL;

    return policyModel;
}

static ocr_model_policy_t * createThorRootModelPolicy ( ) {
    return createEmptyModelPolicyHelper(1);
}

static ocr_model_policy_t * createThorL3ModelPolicies ( size_t n_L3s ) {
    return createEmptyModelPolicyHelper(n_L3s);
}

static ocr_model_policy_t * createThorL2ModelPolicies ( size_t n_L2s ) {
    return createEmptyModelPolicyHelper(n_L2s);
}

static ocr_model_policy_t * createThorL1ModelPolicies ( size_t n_L1s ) {
    return createEmptyModelPolicyHelper(n_L1s);
}

void createThorWorkerModelPoliciesHelper ( ocr_model_policy_t * leafPolicyModel, size_t nb_policy_domains, size_t nb_workers, size_t worker_offset ) {
    int nb_schedulers = 1;
    int nb_comp_targets = 1;
    int nb_workpiles = 1;

    leafPolicyModel->model.nb_instances = nb_policy_domains;
    leafPolicyModel->model.per_type_configuration = NULL;
    leafPolicyModel->model.per_instance_configuration = NULL;

    leafPolicyModel->nb_scheduler_types = 1;
    leafPolicyModel->nb_worker_types = 1;
    leafPolicyModel->nb_comp_target_types = 1;
    leafPolicyModel->nb_workpile_types = 1;
    leafPolicyModel->numMemTypes = 1;
    leafPolicyModel->numAllocTypes = 1;

    // Default allocator
    ocrAllocatorModel_t *defaultAllocator = (ocrAllocatorModel_t*)malloc(sizeof(ocrAllocatorModel_t));
    defaultAllocator->model.per_type_configuration = NULL;
    defaultAllocator->model.per_instance_configuration = NULL;
    defaultAllocator->model.kind = OCR_ALLOCATOR_DEFAULT;
    defaultAllocator->model.nb_instances = 1;
    defaultAllocator->sizeManaged = gHackTotalMemSize;

    leafPolicyModel->allocators = defaultAllocator;

    size_t index_config = 0, n_all_schedulers = nb_schedulers*nb_policy_domains;

    void** scheduler_configurations = malloc(sizeof(scheduler_configuration*)*n_all_schedulers);
    for ( index_config = 0; index_config < n_all_schedulers; ++index_config ) {
        scheduler_configurations[index_config] = (scheduler_configuration*) malloc(sizeof(scheduler_configuration));
        scheduler_configuration* curr_config = (scheduler_configuration*)scheduler_configurations[index_config];
        curr_config->worker_id_begin = worker_offset + ( index_config / nb_schedulers ) * nb_workers;
        curr_config->worker_id_end = worker_offset + ( index_config / nb_schedulers ) * nb_workers + nb_workers - 1;
    }

    leafPolicyModel->schedulers = newModel( OCR_PLACED_SCHEDULER, nb_schedulers, NULL, scheduler_configurations );
    leafPolicyModel->compTargets  = newModel( ocr_comp_target_default_kind, nb_comp_targets, NULL, NULL );
    leafPolicyModel->workpiles  = newModel( ocr_workpile_default_kind, nb_workpiles, NULL, NULL );
    leafPolicyModel->memories   = newModel( OCR_MEMPLATFORM_DEFAULT, 1, NULL, NULL );

    // Defines how ocr modules are bound togethere
    size_t nb_module_mappings = 5;
    ocr_module_mapping_t * defaultMapping =
        (ocr_module_mapping_t *) malloc(sizeof(ocr_module_mapping_t) * nb_module_mappings);
    // Note: this doesn't bind modules magically. You need to have a mapping function defined
    //       and set in the targeted implementation (see ocr_scheduler_hc implementation for reference).
    //       These just make sure the mapping functions you have defined are called
    defaultMapping[0] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_WORKPILE, OCR_SCHEDULER);
    defaultMapping[1] = build_ocr_module_mapping(ONE_TO_ONE_MAPPING, OCR_WORKER, OCR_COMP_TARGET);
    defaultMapping[2] = build_ocr_module_mapping(ONE_TO_MANY_MAPPING, OCR_SCHEDULER, OCR_WORKER);
    defaultMapping[3] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_MEMORY, OCR_ALLOCATOR);
    defaultMapping[4] = build_ocr_module_mapping(MANY_TO_ONE_MAPPING, OCR_SCHEDULER, OCR_POLICY);
    leafPolicyModel->nb_mappings = nb_module_mappings;
    leafPolicyModel->mappings = defaultMapping;

}

static ocr_model_policy_t * createThorMasteredWorkerModelPolicies ( ) {
    ocr_model_policy_t * leafPolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));
    createThorWorkerModelPoliciesHelper(leafPolicyModel, 1, 1, 0);

    leafPolicyModel->model.kind = OCR_MASTERED_LEAF_PLACE_POLICY;

    size_t index_config = 0;
    void** worker_configurations = malloc(sizeof(worker_configuration*));
    worker_configurations[index_config] = (worker_configuration*) malloc(sizeof(worker_configuration));
    worker_configuration* curr_config = (worker_configuration*)worker_configurations[index_config];
    curr_config->worker_id = index_config;

    leafPolicyModel->workers = newModel(ocr_worker_default_kind, 1, NULL, worker_configurations);

    return leafPolicyModel;
}

static ocr_model_policy_t * createThorWorkerModelPolicies ( size_t n_workers ) {
    ocr_model_policy_t * leafPolicyModel = (ocr_model_policy_t *) malloc(sizeof(ocr_model_policy_t));
    createThorWorkerModelPoliciesHelper(leafPolicyModel, n_workers, 1, 1);

    leafPolicyModel->model.kind = OCR_LEAF_PLACE_POLICY;

    size_t index_config = 0;
    void** worker_configurations = malloc(sizeof(worker_configuration*)*n_workers );
    for ( index_config = 0; index_config < n_workers; ++index_config ) {
        worker_configurations[index_config] = (worker_configuration*) malloc(sizeof(worker_configuration));
        worker_configuration* curr_config = (worker_configuration*)worker_configurations[index_config];
        curr_config->worker_id = 1 + index_config;
    }
    leafPolicyModel->workers = newModel(ocr_worker_default_kind, 1, NULL, worker_configurations);

    return leafPolicyModel;
}
