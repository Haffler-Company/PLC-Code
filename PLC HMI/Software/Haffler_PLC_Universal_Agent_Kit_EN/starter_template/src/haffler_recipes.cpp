#include "haffler_recipes.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <string.h>
#include <stdio.h>
#include <esp_log.h>

static const char *TAG = "RECIPES";
HafflerRecipeManager g_recipes;

void HafflerRecipeManager::begin() {
    nvs_handle_t h;
    if (nvs_open("haffler_rcp", NVS_READWRITE, &h) == ESP_OK) {
        size_t req_size = sizeof(m_recipes);
        if (nvs_get_blob(h, "blob", m_recipes, &req_size) != ESP_OK) {
            ESP_LOGI(TAG, "Initializing default recipe profiles");
            memset(m_recipes, 0, sizeof(m_recipes));
            for (int i = 0; i < HAFFLER_MAX_RECIPES; ++i) {
                snprintf(m_recipes[i].name, sizeof(m_recipes[i].name), "Profile %d", i + 1);
                m_recipes[i].params[0] = 100.0f; // Default primary metric (e.g. 100mm)
                m_recipes[i].params[1] = 1.5f;   // Default secondary timer/feed
            }
            nvs_set_blob(h, "blob", m_recipes, sizeof(m_recipes));
            nvs_commit(h);
        }
        nvs_get_u8(h, "act_idx", &m_active_idx);
        if (m_active_idx >= HAFFLER_MAX_RECIPES) m_active_idx = 0;
        nvs_close(h);
    }
}

bool HafflerRecipeManager::save(uint8_t idx) {
    if (idx >= HAFFLER_MAX_RECIPES) return false;
    nvs_handle_t h;
    if (nvs_open("haffler_rcp", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_blob(h, "blob", m_recipes, sizeof(m_recipes));
        nvs_set_u8(h, "act_idx", m_active_idx);
        nvs_commit(h);
        nvs_close(h);
        return true;
    }
    return false;
}

void HafflerRecipeManager::setActive(uint8_t idx) {
    if (idx < HAFFLER_MAX_RECIPES) {
        m_active_idx = idx;
        save(idx);
    }
}

bool HafflerRecipeManager::exportToJson(char* out_buf, size_t max_len) {
    if (!out_buf || max_len < 256) return false;
    HafflerRecipe *r = getActive();
    snprintf(out_buf, max_len, 
             "{\"active\":%d,\"name\":\"%s\",\"p0\":%.2f,\"p1\":%.2f,\"p2\":%.2f,\"flags\":%u}",
             m_active_idx, r->name, r->params[0], r->params[1], r->params[2], r->flags);
    return true;
}
