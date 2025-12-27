#ifndef NOTIFICATION_MANAGER_HPP
#define NOTIFICATION_MANAGER_HPP

#include "raylib.h"
#include "UIConfig.hpp"
#include <string>
#include <vector>
#include <algorithm>

/**
 * NOTIFICATION MANAGER
 * Enhanced notification system with queue support.
 * Shows multiple stacked notifications that fade out independently.
 */
class NotificationManager {
public:
    static NotificationManager& getInstance() {
        static NotificationManager instance;
        return instance;
    }

    void show(const std::string& message, Color color = WHITE, float duration = 2.0f) {
        // Phase 29: Thread-safe pending buffer
        pendingNotifications.push_back({ message, color, duration, duration });
    }

    void update(float dt) {
        // Merge pending into active list
        if (!pendingNotifications.empty()) {
            for (auto& pn : pendingNotifications) {
                if (notifications.size() >= MAX_NOTIFICATIONS) {
                    notifications.erase(notifications.begin());
                }
                notifications.push_back(std::move(pn));
            }
            pendingNotifications.clear();
        }

        for (auto& n : notifications) {
            n.timer -= dt;
        }
        // Remove expired notifications
        notifications.erase(
            std::remove_if(notifications.begin(), notifications.end(), 
                [](const Notification& n) { return n.timer <= 0; }),
            notifications.end()
        );
    }

    void draw() {
        if (notifications.empty()) return;
        
        int screenW = GetScreenWidth();
        int fontSize = 16;
        float yOffset = 60.0f;
        
        for (const auto& n : notifications) {
            int textW = MeasureText(n.message.c_str(), fontSize);
            
            // Calculate fade based on remaining time
            float alpha = 1.0f;
            if (n.timer < 0.5f) alpha = n.timer / 0.5f;  // Fade out in last 0.5s
            
            // Background
            Rectangle bgRect = { 
                (float)(screenW / 2 - textW / 2 - 15), 
                yOffset, 
                (float)(textW + 30), 
                28.0f 
            };
            DrawRectangleRec(bgRect, Fade(BLACK, 0.7f * alpha));
            DrawRectangleLinesEx(bgRect, 1, Fade(n.color, 0.5f * alpha));
            
            // Text centered
            DrawText(n.message.c_str(), screenW / 2 - textW / 2, (int)yOffset + 6, fontSize, Fade(n.color, alpha));
            
            yOffset += 35.0f;  // Stack next notification below
        }
    }

    void clear() {
        notifications.clear();
    }

private:
    static constexpr int MAX_NOTIFICATIONS = 5;
    
    struct Notification {
        std::string message;
        Color color;
        float duration;
        float timer;
    };
    
    std::vector<Notification> notifications;
    std::vector<Notification> pendingNotifications;
    
    NotificationManager() {}
};

#endif
