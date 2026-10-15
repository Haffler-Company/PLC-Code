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
            ESP_LOGI(TAG, "Initializing default bakery recipe profiles");
            memset(m_recipes, 0, sizeof(m_recipes));
            
            // Recipe 0: Barbari Bread
            snprintf(m_recipes[0].name, sizeof(m_recipes[0].name), "بربری");
            m_recipes[0].params[0] = 240.0f; // Mix time: 240 seconds
            m_recipes[0].params[1] = 0.0f;   // Dosing: Flowmeter
            m_recipes[0].params[2] = 14.0f;  // 14.0 Liters
            m_recipes[0].params[3] = 18.0f;  // Max water temp: 18.0 C

            // Recipe 1: Lavash Bread
            snprintf(m_recipes[1].name, sizeof(m_recipes[1].name), "لواش");
            m_recipes[1].params[0] = 180.0f; // Mix time: 180 seconds
            m_recipes[1].params[1] = 0.0f;   // Dosing: Flowmeter
            m_recipes[1].params[2] = 11.5f;  // 11.5 Liters
            m_recipes[1].params[3] = 19.0f;  // Max water temp: 19.0 C

            // Recipe 2: French Baguette
            snprintf(m_recipes[2].name, sizeof(m_recipes[2].name), "باگت");
            m_recipes[2].params[0] = 300.0f; // Mix time: 300 seconds
            m_recipes[2].params[1] = 0.0f;   // Dosing: Flowmeter
            m_recipes[2].params[2] = 13.0f;  // 13.0 Liters
            m_recipes[2].params[3] = 17.5f;  // Max water temp: 17.5 C

            // Recipe 3: Toast Bread (Timer Mode)
            snprintf(m_recipes[3].name, sizeof(m_recipes[3].name), "نان تست");
            m_recipes[3].params[0] = 210.0f; // Mix time: 210 seconds
            m_recipes[3].params[1] = 1.0f;   // Dosing: Timer mode
            m_recipes[3].params[2] = 18.0f;  // 18 seconds water valve
            m_recipes[3].params[3] = 18.0f;  // Max water temp: 18.0 C

            // Recipe 4: Custom Profile
            snprintf(m_recipes[4].name, sizeof(m_recipes[4].name), "سفارشی");
            m_recipes[4].params[0] = 180.0f; // Mix time: 180 seconds
            m_recipes[4].params[1] = 1.0f;   // Dosing: Timer mode
            m_recipes[4].params[2] = 15.0f;  // 15 seconds water valve
            m_recipes[4].params[3] = 20.0f;  // Max water temp: 20.0 C

            for (int i = 5; i < HAFFLER_MAX_RECIPES; ++i) {
                snprintf(m_recipes[i].name, sizeof(m_recipes[i].name), "Profile %d", i + 1);
                m_recipes[i].params[0] = 180.0f;
                m_recipes[i].params[1] = 0.0f;
                m_recipes[i].params[2] = 12.0f;
                m_recipes[i].params[3] = 18.0f;
            }
            nvs_set_blob(h, "blob", m_recipes, sizeof(m_recipes));
            nvs_commit(h);
        } else {
            // Sanitize names: if any contain '(', strip it
            for (int i = 0; i < HAFFLER_MAX_RECIPES; ++i) {
                char* p = strchr(m_recipes[i].name, '(');
                if (p) {
                    while (p > m_recipes[i].name && *(p - 1) == ' ') p--;
                    *p = '\0';
                }
            }
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
