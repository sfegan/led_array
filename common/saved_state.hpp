#pragma once

#include <cstdint>
#include <vector>

class SavedStateSupplierConsumer {
public:
    virtual ~SavedStateSupplierConsumer();
    virtual std::vector<int32_t> get_saved_state() = 0;
    virtual bool set_saved_state(const std::vector<int32_t>& state) = 0;
    virtual int32_t get_version() = 0;
    virtual int32_t get_supplier_id() = 0;
};

class SavedStateManager {
public:
    SavedStateManager(unsigned max_state_size_words = 1024);
    ~SavedStateManager();
    void add_saved_state_supplier(SavedStateSupplierConsumer* supplier);
    bool save_state();
    bool load_state();
    virtual int32_t get_application_id() = 0;

    const std::vector<int32_t>& state() const { return state_; }
private:
    unsigned flash_target_size_;
    unsigned flash_target_offset_;
    const int32_t* base_;

    std::vector<SavedStateSupplierConsumer*> suppliers_;
    std::vector<int32_t> state_;

    static void call_flash_range_erase(void *param);
    static void call_flash_range_program(void *param);
};
