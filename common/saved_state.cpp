#include <vector>

#include "pico/flash.h"
#include "hardware/flash.h"

#include "build_date.hpp"
#include "saved_state.hpp"

namespace {
    static BuildDate build_date(__DATE__,__TIME__);
}

SavedStateSupplierConsumer::~SavedStateSupplierConsumer()
{
    // nothing to see here
}

// Note the last sector of the flash is erased systematically by the UF2 
// bootloader as there is something stored there (as seen with uf2conv.py). 
// So we use the second last sector which seems to be unused by the bootloader.
// See RP2350-E10 erratum for more details.

SavedStateManager::SavedStateManager(unsigned max_state_size_words):
    flash_target_size_(((max_state_size_words+1023)/1024)*1024*sizeof(int32_t)),
    flash_target_offset_(PICO_FLASH_SIZE_BYTES - 4096 - flash_target_size_),
    base_(reinterpret_cast<const int32_t *>(XIP_BASE + flash_target_offset_)),
    suppliers_(),
    state_(flash_target_size_/sizeof(int32_t), 0)
{
    // nothing to see here
}

SavedStateManager::~SavedStateManager()
{
    // nothing to see here
}

void SavedStateManager::add_saved_state_supplier(SavedStateSupplierConsumer* supplier)
{
    if(supplier) {
        suppliers_.push_back(supplier);
    }
}

// This function will be called when it's safe to call flash_range_erase
void SavedStateManager::call_flash_range_erase(void *param) {
    SavedStateManager* ssm = static_cast<SavedStateManager*>(param);
    flash_range_erase(ssm->flash_target_offset_, ssm->flash_target_size_);
}

// This function will be called when it's safe to call flash_range_program
void SavedStateManager::call_flash_range_program(void *param) {
    SavedStateManager* ssm = static_cast<SavedStateManager*>(param);
    flash_range_program(ssm->flash_target_offset_, 
        reinterpret_cast<const uint8_t*>(ssm->state_.data()), ssm->flash_target_size_);
}

bool SavedStateManager::save_state(bool multicore)
{
    std::fill(state_.begin(), state_.end(), 0);
    unsigned istate = 0;
    state_[istate++] = get_application_id();
    for(auto& supplier : suppliers_) {
        auto s = supplier->get_saved_state();
        if((istate + 3 + s.size()) > flash_target_size_) {
            return false;
        }
        state_[istate++] = supplier->get_supplier_id();
        state_[istate++] = supplier->get_version();
        state_[istate++] = s.size();
        for(auto& is : s) {
            state_[istate++] = is;
        }
    }

    if(multicore) {
        // Flash is "execute in place" and so will be in use when any code that is stored in flash runs, e.g. an interrupt handler
        // or code running on a different core.
        // Calling flash_range_erase or flash_range_program at the same time as flash is running code would cause a crash.
        // flash_safe_execute disables interrupts and tries to cooperate with the other core to ensure flash is not in use
        // See the documentation for flash_safe_execute and its assumptions and limitations

        int rc = flash_safe_execute(call_flash_range_erase, (void*)this, UINT32_MAX);
        if(rc != PICO_OK) {
            printf("Flash erase failed: %d\n", rc);
            return false;
        }

        rc = flash_safe_execute(call_flash_range_program, (void*)this, UINT32_MAX);
        if(rc != PICO_OK) {
            printf("Flash program failed: %d\n", rc);
            return false;
        }
    } else {
        flash_range_erase(flash_target_offset_, flash_target_size_);
        flash_range_program(flash_target_offset_, 
            reinterpret_cast<const uint8_t*>(state_.data()), flash_target_size_);
    }

    return true;
}

bool SavedStateManager::load_state(bool debug)
{
    std::copy(base_, base_ + state_.size(), state_.begin());
    unsigned istate = 0;
    if(state_[istate++] != get_application_id()) {
        if(debug) {
            printf("Saved state application ID mismatch: %d != %d\n", 
                state_[istate-1], get_application_id());
        }
        return false;
    }
    while(istate < state_.size()) {
        int32_t supplier_id = state_[istate++];
        if(supplier_id == 0) {
            break;
        }
        if(istate + 2 > state_.size()) {
            return false;
        }
        int32_t version = state_[istate++];
        int32_t size = state_[istate++];
        if(istate + size > state_.size()) {
            printf("Saved state size mismatch: %d > %d\n", 
                istate + size, state_.size());
            return false;
        }
        std::vector<int32_t> s(size);
        for(int i=0; i<size; ++i) {
            s[i] = state_[istate++];
        }
        bool found = false;
        bool loaded = false;
        for(auto& supplier : suppliers_) {
            if(supplier->get_supplier_id() == supplier_id 
                and supplier->get_version() == version) 
            {
                found = true;
                loaded = supplier->set_saved_state(s);
            }
        }
        if(debug) {
            char code[5] = {0,0,0,0,0};
            *reinterpret_cast<int32_t*>(code) = supplier_id;
            printf("State %s, ver=%d, size=%d, %s\n", 
                code, version, size, loaded ? "loaded" : (found ? "not loaded" : "not found"));
        }
    }
    return true;
}
