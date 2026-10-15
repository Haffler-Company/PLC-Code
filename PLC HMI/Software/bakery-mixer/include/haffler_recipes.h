#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define HAFFLER_MAX_RECIPES 16

struct HafflerRecipe {
    char name[32];
    float params[8];
    uint16_t flags;
};

class HafflerRecipeManager {
private:
    HafflerRecipe m_recipes[HAFFLER_MAX_RECIPES];
    uint8_t m_active_idx = 0;

public:
    void begin();
    bool save(uint8_t idx);
    bool load(uint8_t idx);
    void setActive(uint8_t idx);
    uint8_t getActiveIndex() const { return m_active_idx; }
    HafflerRecipe* getActive() { return &m_recipes[m_active_idx]; }
    HafflerRecipe* get(uint8_t idx) { return (idx < HAFFLER_MAX_RECIPES) ? &m_recipes[idx] : nullptr; }
    float get_param(uint8_t p_idx) const { return (p_idx < 8) ? m_recipes[m_active_idx].params[p_idx] : 0.0f; }
    void set_param(uint8_t p_idx, float val) { if (p_idx < 8) m_recipes[m_active_idx].params[p_idx] = val; }
    
    // JSON export/import for embedded WebPortal
    bool exportToJson(char* out_buf, size_t max_len);
    bool importFromJson(const char* json_str);
};

extern HafflerRecipeManager g_recipes;
