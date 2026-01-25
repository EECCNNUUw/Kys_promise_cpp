#include "UIManager.h"
#include "GameManager.h"
#include "PicLoader.h"
#include "GraphicsUtils.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <cmath>
#include <algorithm>

UIManager& UIManager::getInstance() {
    static UIManager instance;
    return instance;
}

UIManager::UIManager() {
    if (!TTF_Init()) {
        std::cerr << "TTF_Init Error: " << SDL_GetError() << std::endl;
    }
}

bool UIManager::Init(SDL_Renderer* renderer, SDL_Window* window) {
    m_renderer = renderer;
    m_window = window;
    // Load font - Try multiple paths
    const char* fontPaths[] = {
        "resource/simkai.ttf",
        "C:/Windows/Fonts/simkai.ttf",
        "resource/font.ttf"
    };
    
    for (const auto& path : fontPaths) {
        m_font = TTF_OpenFont(path, 20);
        if (m_font) break;
    }
    
    if (!m_font) {
        std::cerr << "Failed to load font from any known path!" << std::endl;
        // return false; // Don't fail hard for now
    }
    return true;
}

void UIManager::Cleanup() {
    if (m_font) {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }
    if (m_texTitle) SDL_DestroyTexture(m_texTitle);
    if (m_texMagic) SDL_DestroyTexture(m_texMagic);
    if (m_texState) SDL_DestroyTexture(m_texState);
    if (m_texSystem) SDL_DestroyTexture(m_texSystem);
    if (m_texMap) SDL_DestroyTexture(m_texMap);
    if (m_texSkill) SDL_DestroyTexture(m_texSkill);
    if (m_texMenuEsc) SDL_DestroyTexture(m_texMenuEsc);
    if (m_texMenuEscBack) SDL_DestroyTexture(m_texMenuEscBack);
    if (m_texBattle) SDL_DestroyTexture(m_texBattle);
    if (m_texTeammate) SDL_DestroyTexture(m_texTeammate);
    if (m_texMenuItem) SDL_DestroyTexture(m_texMenuItem);
    if (m_texMenuBackground) SDL_DestroyTexture(m_texMenuBackground);
    
    TTF_Quit();
}

bool UIManager::LoadSystemGraphics() {
    if (m_texTitle) return true; 

    auto loadTex = [&](int index) -> SDL_Texture* {
        PicImage pic = PicLoader::loadPic("resource/Background.Pic", index);
        if (pic.surface) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, pic.surface);
            PicLoader::freePic(pic);
            return tex;
        }
        return nullptr;
    };

    m_texTitle = loadTex(0);
    m_texMagic = loadTex(1);
    m_texState = loadTex(2);
    m_texSystem = loadTex(3);
    m_texMap = loadTex(4);
    m_texSkill = loadTex(5);
    m_texMenuEsc = loadTex(6);
    m_texMenuEscBack = loadTex(7);
    m_texBattle = loadTex(8);
    m_texTeammate = loadTex(9);
    m_texMenuItem = loadTex(10);
    
    return true;
}

void UIManager::CaptureScreen() {
    if (m_texMenuBackground) {
        SDL_DestroyTexture(m_texMenuBackground);
        m_texMenuBackground = nullptr;
    }
    // In SDL3, use SDL_RenderReadPixels to get surface
    SDL_Surface* surface = SDL_RenderReadPixels(m_renderer, NULL);
    if (surface) {
        m_texMenuBackground = SDL_CreateTextureFromSurface(m_renderer, surface);
        SDL_DestroySurface(surface);
    }
}

void UIManager::DrawRectangle(int x, int y, int w, int h, uint32_t colorin, uint32_t colorframe, int alpha) {
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    
    // Fill
    SDL_SetRenderDrawColor(m_renderer, (colorin >> 24) & 0xFF, (colorin >> 16) & 0xFF, (colorin >> 8) & 0xFF, alpha);
    SDL_FRect rect = { (float)x, (float)y, (float)w, (float)h };
    SDL_RenderFillRect(m_renderer, &rect);
    
    // Frame
    SDL_SetRenderDrawColor(m_renderer, (colorframe >> 24) & 0xFF, (colorframe >> 16) & 0xFF, (colorframe >> 8) & 0xFF, 255);
    SDL_RenderRect(m_renderer, &rect);
}

void UIManager::DrawShadowTextUtf8(const std::string& text, int x, int y, uint32_t color1, uint32_t color2, int fontSize) {
    if (!m_font) return;
    
    // Using RRGGBBAA format logic:
    // color >> 24 is R
    SDL_Color c2 = { (Uint8)((color2 >> 24) & 0xFF), (Uint8)((color2 >> 16) & 0xFF), (Uint8)((color2 >> 8) & 0xFF), (Uint8)(color2 & 0xFF) };
    SDL_Color c1 = { (Uint8)((color1 >> 24) & 0xFF), (Uint8)((color1 >> 16) & 0xFF), (Uint8)((color1 >> 8) & 0xFF), (Uint8)(color1 & 0xFF) };
    
    SDL_Surface* surf2 = TTF_RenderText_Solid(m_font, text.c_str(), 0, c2);
    if (surf2) {
        SDL_Texture* tex2 = SDL_CreateTextureFromSurface(m_renderer, surf2);
        SDL_FRect dst2 = { (float)(x + 1), (float)(y + 1), (float)surf2->w, (float)surf2->h };
        SDL_RenderTexture(m_renderer, tex2, NULL, &dst2);
        SDL_DestroyTexture(tex2);
        SDL_DestroySurface(surf2);
    }
    
    SDL_Surface* surf1 = TTF_RenderText_Solid(m_font, text.c_str(), 0, c1);
    if (surf1) {
        SDL_Texture* tex1 = SDL_CreateTextureFromSurface(m_renderer, surf1);
        SDL_FRect dst1 = { (float)x, (float)y, (float)surf1->w, (float)surf1->h };
        SDL_RenderTexture(m_renderer, tex1, NULL, &dst1);
        SDL_DestroyTexture(tex1);
        SDL_DestroySurface(surf1);
    }
}

void UIManager::DrawHead(int headId, int x, int y) {
    PicImage pic = PicLoader::loadPic("resource/Heads.Pic", headId);
    if (pic.surface) {
         SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, pic.surface);
         SDL_FRect dest = { (float)x, (float)y, (float)pic.surface->w, (float)pic.surface->h };
         SDL_RenderTexture(m_renderer, tex, NULL, &dest);
         SDL_DestroyTexture(tex);
         PicLoader::freePic(pic);
    }
}

void UIManager::RenderMenuSystem(int menuSelection) {
    if (m_texMenuEscBack) {
        // 底图贴图 (Index 7) 包含两个横向并列的 300x300 圆环
        // Pascal 源码中使用 (0, 0, 300, 300) 作为显示底图
        SDL_FRect src = { 0, 0, 300, 300 };
        SDL_FRect dest = { 170, 70, 300, 300 };
        SDL_RenderTexture(m_renderer, m_texMenuEscBack, &src, &dest);
    }
    
    int x = 270;
    int y = 167;
    int N = 102;
    
    int positionX[6] = { x, x + N, x + N, x, x - N, x - N };
    int positionY[6] = { y - 117, y - 58, y + 58, y + 117, y + 58, y - 58 };
    
    for (int i = 0; i < 6; ++i) {
        float srcX = (float)((i % 3) * 100);
        float srcY = (float)((i / 3) * 100);
        
        if (i != menuSelection) {
            srcY += 200; // Not selected
        }
        
        SDL_FRect src = { srcX, srcY, 100, 100 };
        SDL_FRect dest = { (float)positionX[i], (float)positionY[i], 100, 100 };
        
        if (m_texMenuEsc) {
            SDL_RenderTexture(m_renderer, m_texMenuEsc, &src, &dest);
        }
    }
}

void UIManager::ShowMenu() {
    if (!m_texMenuEsc) LoadSystemGraphics();
    
    CaptureScreen();

    bool running = true;
    int currentSelection = 0;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                 if (event.key.key == SDLK_ESCAPE) {
                     running = false;
                 }
                 if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                     currentSelection = (currentSelection + 1) % 6;
                 } else if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                     currentSelection = (currentSelection + 5) % 6;
                 } else if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) {
                     // Approximate Pascal right logic
                      if (currentSelection == 0) currentSelection = 1;
                      else if (currentSelection == 3 || currentSelection == 4) currentSelection--;
                      else if (currentSelection == 5) currentSelection = 0;
                 } else if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) {
                     // Approximate Pascal left logic
                     if (currentSelection == 0) currentSelection = 5;
                     else if (currentSelection == 2 || currentSelection == 3) currentSelection++;
                     else if (currentSelection == 5 || currentSelection == 1) currentSelection--;
                 }
                 
                 if (event.key.key == SDLK_SPACE || event.key.key == SDLK_RETURN) {
                     if (currentSelection == 0) SelectShowMagic();
                     else if (currentSelection == 1) SelectShowStatus();
                     else if (currentSelection == 2) SelectShowSystem();
                     else if (currentSelection == 3) SelectShowTeammate();
                     else if (currentSelection == 4) SelectShowSkill();
                     else if (currentSelection == 5) SelectShowItem();
                 }
            }
        }

        SDL_RenderClear(m_renderer);
        
        if (m_texMenuBackground) {
             SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        }
        
        RenderMenuSystem(currentSelection);
        
        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::SelectShowStatus() {
    auto& game = GameManager::getInstance();
    const auto& team = game.getTeamList();
    if (team.empty()) return;

    int currentIdx = 0;
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                game.Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_UP:
                    case SDLK_KP_8:
                    case SDLK_LEFT:
                        currentIdx--;
                        if (currentIdx < 0) currentIdx = team.size() - 1;
                        break;
                    case SDLK_DOWN:
                    case SDLK_KP_2:
                    case SDLK_RIGHT:
                        currentIdx++;
                        if (currentIdx >= (int)team.size()) currentIdx = 0;
                        break;
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                }
            }
        }

        SDL_RenderClear(m_renderer);
        if (m_texMenuBackground) {
            SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        }

        ShowStatus(team[currentIdx]);

        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::ShowStatus(int roleId) {
    if (!m_texState) LoadSystemGraphics();

    if (m_texState) {
        SDL_RenderTexture(m_renderer, m_texState, NULL, NULL);
    }
    
    Role& role = GameManager::getInstance().getRole(roleId);
    const auto& team = GameManager::getInstance().getTeamList();

    // Draw Team List
    DrawRectangle(15, 15, 90, 10 + team.size() * 22, 0x00000000, 0xFFFFFFFF, 30);
    for (size_t i = 0; i < team.size(); ++i) {
        int id = team[i];
        Role& r = GameManager::getInstance().getRole(id);
        uint32_t color = (id == roleId) ? 0xFFFFFFFF : 0xFFFF00FF;
        DrawShadowTextUtf8(r.getName(), 20, 20 + i * 22, color, 0x000000FF);
    }

    DrawHead(role.getHeadNum(), 137, 88);
    DrawShadowTextUtf8(role.getName(), 115, 93, 0xFFFFFFFF, 0x000000FF);
    
    // TODO: Complete Status fields
    DrawShadowTextUtf8("生命", 125, 125, 0xFFFFFFFF, 0x000000FF);
    DrawShadowTextUtf8(std::to_string(role.getCurrentHP()), 165, 125, 0xFFFFFFFF, 0x000000FF);
}

void UIManager::SelectShowMagic() {
    if (!m_texMagic) LoadSystemGraphics();
    
    auto& game = GameManager::getInstance();
    const auto& team = game.getTeamList();
    if (team.empty()) return;

    int currentIdx = 0;
    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                game.Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                switch (event.key.key) {
                    case SDLK_UP:
                    case SDLK_KP_8:
                    case SDLK_LEFT:
                        currentIdx--;
                        if (currentIdx < 0) currentIdx = team.size() - 1;
                        break;
                    case SDLK_DOWN:
                    case SDLK_KP_2:
                    case SDLK_RIGHT:
                        currentIdx++;
                        if (currentIdx >= (int)team.size()) currentIdx = 0;
                        break;
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                }
            }
        }

        SDL_RenderClear(m_renderer);
        if (m_texMenuBackground) {
            SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        }

        ShowMagic(team[currentIdx]);

        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::ShowMagic(int roleId, int selectedIndex) {
    if (!m_texMagic) LoadSystemGraphics();

    if (m_texMagic) {
        SDL_RenderTexture(m_renderer, m_texMagic, NULL, NULL);
    }

    Role& role = GameManager::getInstance().getRole(roleId);
    const auto& team = GameManager::getInstance().getTeamList();

    // Draw Team List
    DrawRectangle(15, 15, 90, 10 + team.size() * 22, 0x00000000, 0xFFFFFFFF, 30);
    for (size_t i = 0; i < team.size(); ++i) {
        int id = team[i];
        Role& r = GameManager::getInstance().getRole(id);
        uint32_t color = (id == roleId) ? 0xFFFFFFFF : 0xFFFF00FF;
        DrawShadowTextUtf8(r.getName(), 20, 20 + i * 22, color, 0x000000FF);
    }

    DrawHead(role.getHeadNum(), 137, 88);
    DrawShadowTextUtf8(role.getName(), 115, 93, 0xFFFFFFFF, 0x000000FF);

    int x = 90;
    int y = 0;

    // UpdateHpMp Logic
    const char* hpMpLabels[] = { " 生命", " 內力", " 體力" };
    for (int i = 0; i < 3; ++i) {
        DrawShadowTextUtf8(hpMpLabels[i], x + 25, y + 94 + 21 * (i + 1), 0xFFD700FF, 0x000000FF);
    }

    // HP
    DrawShadowTextUtf8(std::to_string(role.getCurrentHP()), x + 125 + 25, y + 94 + 21, 0xFFFFFFFF, 0x000000FF);
    DrawShadowTextUtf8("/", x + 165 + 25, y + 94 + 21, 0xFFFFFFFF, 0x000000FF);
    DrawShadowTextUtf8(std::to_string(role.getMaxHP()), x + 175 + 25, y + 94 + 21, 0xFFFFFFFF, 0x000000FF);
    
    // MP
    DrawShadowTextUtf8(std::to_string(role.getCurrentMP()) + "/" + std::to_string(role.getMaxMP()), x + 125 + 25, y + 94 + 21 * 2, 0xFFFFFFFF, 0x000000FF);
    
    // PhyPower
    DrawShadowTextUtf8(std::to_string(role.getPhyPower()) + "/100", x + 125 + 25, y + 94 + 21 * 3, 0xFFFFFFFF, 0x000000FF);

    // Practice Item
    DrawShadowTextUtf8(" 修煉物品", x + 110, y + 216, 0xFFD700FF, 0x000000FF);
    if (role.getPracticeBook() >= 0) {
        Item& book = GameManager::getInstance().getItem(role.getPracticeBook());
        DrawShadowTextUtf8(book.getName(), x + 110, y + 237, 0xFFFFFFFF, 0x000000FF);
        
        // Draw Item Pic
        PicImage pic = PicLoader::loadPic("resource/Items.Pic", role.getPracticeBook());
        if (pic.surface) {
             SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, pic.surface);
             SDL_FRect dest = { (float)136, (float)208, (float)pic.surface->w, (float)pic.surface->h };
             SDL_RenderTexture(m_renderer, tex, NULL, &dest);
             SDL_DestroyTexture(tex);
             PicLoader::freePic(pic);
        }
        
        DrawShadowTextUtf8(std::to_string(role.getExpForBook()), x + 137, y + 258, 0xFFFFFFFF, 0x000000FF);
    } else {
        DrawShadowTextUtf8(" 無", x + 110, y + 237, 0xFFFFFFFF, 0x000000FF);
    }

    // Gongti Exp
    DrawShadowTextUtf8(" 功體經驗", x + 25, y + 184, 0xFFD700FF, 0x000000FF);
    DrawShadowTextUtf8(std::to_string(role.getGongtiExam()), x + 137, y + 184, 0xFFFFFFFF, 0x000000FF);

    // Special Skills (Top Right)
    const char* skills[] = { " 醫療", " 解毒", " 用毒", " 抗毒", " 毒攻", " " };
    for (int i = 0; i < 5; ++i) {
        DrawShadowTextUtf8(skills[i], x + 248 + 78 * (i % 3), y + (i / 3) * 22 + 58, 0xFFFFFFFF, 0x000000FF);
    }

    // Magic List
    DrawShadowTextUtf8(" ————所會武功————", x + 247, y + 102, 0xFFD700FF, 0x000000FF);
    for (int i = 0; i < 10; ++i) {
        int magicId = role.getMagic(i);
        if (magicId > 0) {
            Magic& m = GameManager::getInstance().getMagic(magicId);
            DrawShadowTextUtf8(m.getName(), x + 248 + 118 * (i % 2), y + (i / 2) * 22 + 124, 0xFFFFFFFF, 0x000000FF);
        }
    }
}

void UIManager::SelectShowSystem() {
    bool running = true;
    int currentSelection = 0;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                    currentSelection = (currentSelection + 1) % 4;
                } else if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                    currentSelection = (currentSelection + 3) % 4;
                } else if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        SDL_RenderClear(m_renderer);
        if (m_texMenuBackground) {
            SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        }
        ShowSystem(currentSelection);
        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::ShowSystem(int selectedIndex) {
    if (!m_texSystem) LoadSystemGraphics();
    if (m_texSystem) {
        SDL_RenderTexture(m_renderer, m_texSystem, NULL, NULL);
    }

    const char* labels[] = {
        " ——————————讀取進度——————————",
        " ——————————保存進度——————————",
        " ——————————音樂音量——————————",
        " ——————————退出離開——————————"
    };

    for (int i = 0; i < 4; ++i) {
        uint32_t color = (i == selectedIndex) ? 0xFFFFFFFF : 0xAAAAAAFF;
        DrawShadowTextUtf8(labels[i], 112, 25 + 101 * i, color, 0x000000FF);
    }
}

void UIManager::SelectShowSkill() {
    bool running = true;
    int currentPet = 0; // 0 for player, 1-N for pets
    int currentIdx = 0;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
                // Simplified navigation for pets
                if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                    currentIdx++;
                } else if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                    currentIdx--;
                }
            }
        }

        SDL_RenderClear(m_renderer);
        if (m_texMenuBackground) {
            SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        }
        ShowSkill(currentPet, currentIdx);
        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::ShowSkill(int petId, int selectedIndex) {
    if (!m_texSkill) LoadSystemGraphics();
    if (m_texSkill) {
        SDL_RenderTexture(m_renderer, m_texSkill, NULL, NULL);
    }
    // TODO: Implement actual pet display logic from Pascal
    DrawShadowTextUtf8(" ————目前尚無寵物————", 120, 50, 0xFFFFFFFF, 0x000000FF);
}

void UIManager::SelectShowTeammate() {
    bool running = true;
    int tMenu = 1;
    int rMenu = 0;
    int position = 0;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
                // Basic navigation
                if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                    if (position == 0) { tMenu = (tMenu % 5) + 1; }
                    else { rMenu = (rMenu + 2) % 26; }
                } else if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                    if (position == 0) { tMenu = (tMenu == 1) ? 5 : tMenu - 1; }
                    else { rMenu = (rMenu == 0) ? 24 : rMenu - 2; }
                } else if (event.key.key == SDLK_RIGHT || event.key.key == SDLK_KP_6) {
                    position = 1;
                } else if (event.key.key == SDLK_LEFT || event.key.key == SDLK_KP_4) {
                    position = 0;
                }
            }
        }

        SDL_RenderClear(m_renderer);
        if (m_texMenuBackground) {
            SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        }
        ShowTeammate(tMenu, rMenu, position);
        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::ShowTeammate(int tMenu, int rMenu, int position) {
    if (!m_texTeammate) LoadSystemGraphics();
    if (m_texTeammate) {
        SDL_RenderTexture(m_renderer, m_texTeammate, NULL, NULL);
    }

    int x1 = 120;
    int x2 = 350;
    int y1 = 35;
    int y2 = 35;

    DrawRectangle(x1 + 15, y1 - 5, 220, 160, 0, 0xFFFFFFFF, 40);
    DrawShadowTextUtf8(" 隊中人員", x1, y1, 0xFFFFFFFF, 0x000000FF);
    
    DrawRectangle(x2 + 15, y2 - 5, 240, 376, 0, 0xFFFFFFFF, 40);
    DrawShadowTextUtf8(" 預備人員", x2, y2, 0xFFFFFFFF, 0x000000FF);

    const auto& team = GameManager::getInstance().getTeamList();
    for (size_t i = 0; i < team.size() && i < 6; ++i) {
        Role& r = GameManager::getInstance().getRole(team[i]);
        uint32_t color = (position == 0 && (int)i == tMenu - 1) ? 0xFFFFFFFF : 0xAAAAAAFF;
        DrawShadowTextUtf8(r.getName(), x1 + 20, y1 + 30 + i * 25, color, 0x000000FF);
    }
}

void UIManager::SelectShowItem() {
    bool running = true;
    int menuSelection = 0;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
                if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                    menuSelection = (menuSelection + 1) % 6;
                } else if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                    menuSelection = (menuSelection + 5) % 6;
                }
            }
        }

        SDL_RenderClear(m_renderer);
        if (m_texMenuBackground) {
            SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        }
        ShowItem(menuSelection);
        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::ShowItem(int menuSelection) {
    if (!m_texMenuItem) LoadSystemGraphics();
    if (m_texMenuItem) {
        SDL_RenderTexture(m_renderer, m_texMenuItem, NULL, NULL);
    }

    const char* labels[] = { " 全部物品", " 劇情物品", " 神兵寶甲", " 武功秘笈", " 靈丹妙藥", " 傷人暗器" };
    int x = 15, y = 15;
    DrawRectangle(x, y, 90, 6 * 22 + 28, 0, 0xFFFFFFFF, 30);

    for (int i = 0; i < 6; ++i) {
        uint32_t color = (i == menuSelection) ? 0xFFFFFFFF : 0xAAAAAAFF;
        DrawShadowTextUtf8(labels[i], x + 5, y + 5 + 22 * i, color, 0x000000FF);
    }
}

// Stubs for missing implementations
void UIManager::PlayTitleAnimation() {}
void UIManager::DrawTitleScreen() {}
void UIManager::DrawTitleBackground() {}
void UIManager::DrawCenteredTexture(SDL_Texture* tex) {}
void UIManager::DrawText(const std::string& text, int x, int y, uint32_t color, int fontSize) {}
void UIManager::DrawTextUtf8(const std::string& text, int x, int y, uint32_t color, int fontSize) {}
void UIManager::DrawShadowText(const std::string& text, int x, int y, uint32_t color1, uint32_t color2, int fontSize) {}
void UIManager::ShowCharacterCreation(const Role& role) {}
void UIManager::ShowSaveLoadMenu(bool isSave) {}
void UIManager::ShowDialogue(const std::string& text, int headId, int mode, const std::string& name) {}
void UIManager::ShowTitle(const std::string& text, int x, int y, uint32_t color1, uint32_t color2) {}
int UIManager::ShowChoice(const std::string& text) { return 0; }
void UIManager::ShowItemNotification(int itemId, int amount) {}
void UIManager::FadeScreen(bool fadeIn) {}
void UIManager::FlashScreen(uint32_t color, int durationMs) {}
void UIManager::UpdateScreen() { SDL_RenderPresent(m_renderer); }
