#include "UIManager.h"
#include "GameManager.h"
#include "PicLoader.h"
#include "TextManager.h"
#include "GraphicsUtils.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <cmath>
#include <algorithm>

namespace {
    std::string g_loadedFontPath;
}

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
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/msjh.ttc",
        "C:/Windows/Fonts/simsun.ttc",
        "C:/Windows/Fonts/simsun.ttf",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/kaiu.ttf",
        "C:/Windows/Fonts/mingliu.ttc",
        "resource/simkai.ttf",
        "C:/Windows/Fonts/simkai.ttf",
        "resource/font.ttf"
    };
    
    for (const auto& path : fontPaths) {
        m_font = TTF_OpenFont(path, 20);
        if (m_font) {
            g_loadedFontPath = path;
            std::cout << "[UIManager] Loaded font: " << path << std::endl;
            break;
        }
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

SDL_Color UIManager::Uint32ToColor(uint32_t color) {
    SDL_Color c;
    c.r = (color >> 16) & 0xFF;
    c.g = (color >> 8) & 0xFF;
    c.b = color & 0xFF;
    c.a = (color >> 24) & 0xFF;
    return c;
}

bool UIManager::LoadSystemGraphics() {
    if (m_texTitle) return true; 

    auto loadTex = [&](int index) -> SDL_Texture* {
        PicImage pic = PicLoader::loadPic("resource/Background.Pic", index);
        if (pic.surface) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, pic.surface);
            if (tex) {
                SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND); // 确保透明通道生效
            }
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
    if (headId < 0) return; // Prevent invalid head index

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

    const int infoX = 122;
    const int infoW = 499;
    DrawRectangle(infoX, 16, infoW, 25, 0, 0xFFFFFFFF, 40);
    DrawRectangle(infoX, 46, infoW, 25, 0, 0xFFFFFFFF, 40);
    DrawRectangle(infoX, 76, infoW, 252, 0, 0xFFFFFFFF, 40);
    DrawRectangle(infoX, 335, infoW, 86, 0, 0xFFFFFFFF, 40);

    const int cols = 6;
    const int rows = 3;
    const int cellW = 82;
    const int cellH = 82;
    const int gridX = 115 + 12;
    const int gridY = 95 - 14;
    const int maxCells = cols * rows;

    std::vector<InventoryItem> filtered;
    const auto& inventory = GameManager::getInstance().getItemList();
    int filterType = (menuSelection == 0) ? 100 : (menuSelection - 1);
    for (const auto& it : inventory) {
        if (it.id < 0 || it.amount <= 0) continue;
        Item& item = GameManager::getInstance().getItem(it.id);
        if (filterType == 100 || item.getItemType() == filterType) {
            filtered.push_back(it);
        }
    }

    for (int i = 0; i < maxCells && i < (int)filtered.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int itemId = filtered[i].id;
        PicImage pic = PicLoader::loadPic("resource/Items.Pic", itemId);
        if (pic.surface) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, pic.surface);
            if (tex) {
                SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                SDL_FRect dest = { (float)(gridX + col * cellW), (float)(gridY + row * cellH), (float)pic.surface->w, (float)pic.surface->h };
                SDL_RenderTexture(m_renderer, tex, NULL, &dest);
                SDL_DestroyTexture(tex);
            }
            PicLoader::freePic(pic);
        }
    }

    if (!filtered.empty()) {
        int selectedIndex = 0;
        int selCol = selectedIndex % cols;
        int selRow = selectedIndex / cols;
        DrawRectangle(gridX + selCol * cellW - 2, gridY + selRow * cellH - 2, cellW - 4, cellH - 4, 0, 0xFFFFFFFF, 0);

        Item& item = GameManager::getInstance().getItem(filtered[selectedIndex].id);
        std::string nameUtf8 = TextManager::getInstance().gbkToUtf8(item.getName());
        std::string introUtf8 = TextManager::getInstance().gbkToUtf8(item.getIntroduction());
        std::string amountStr = std::to_string(filtered[selectedIndex].amount);
        DrawShadowTextUtf8(nameUtf8, 134, 20, 0xFFFF00FF, 0x000000FF);
        DrawShadowTextUtf8(introUtf8, 134, 50, 0xFFFFFFFF, 0x000000FF);
        DrawShadowTextUtf8(" 數量", 430, 20, 0xFFFF00FF, 0x000000FF);
        DrawShadowTextUtf8(amountStr, 490, 20, 0xFFFFFFFF, 0x000000FF);

        const char* typeLabels[] = { " 劇情物品", " 神兵寶甲", " 武功秘笈", " 靈丹妙藥", " 傷人暗器" };
        int t = item.getItemType();
        if (t >= 0 && t <= 4) {
            DrawShadowTextUtf8(typeLabels[t], 134, 350, 0xFFFFFFFF, 0x000000FF);
        }
    }
}

// Stubs for missing implementations
void UIManager::PlayTitleAnimation() {
    int frameCount = PicLoader::getPicCount("resource/Begin.Pic");
    if (frameCount <= 0) return;

    bool skip = false;
    SDL_Event event;

    // 播放开场动画 (Begin.Pic)
    for (int i = 0; i < frameCount && !skip; ++i) {
        // 检测跳过输入
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                skip = true;
                break;
            }
            if (event.type == SDL_EVENT_KEY_UP) {
                if (event.key.key == SDLK_ESCAPE || event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                    skip = true;
                    break;
                }
            }
        }

        PicImage pic = PicLoader::loadPic("resource/Begin.Pic", i);
        if (pic.surface) {
            SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, pic.surface);
            if (tex) {
                SDL_RenderClear(m_renderer);
                
                // 模拟 Pascal 的 ZoomPic 逻辑，拉伸至全屏
                int w, h;
                SDL_GetWindowSize(m_window, &w, &h);
                SDL_FRect dest = { 0.0f, 0.0f, (float)w, (float)h };
                SDL_RenderTexture(m_renderer, tex, NULL, &dest);
                
                SDL_RenderPresent(m_renderer);
                SDL_DestroyTexture(tex);
            }
            PicLoader::freePic(pic);
        }
        SDL_Delay(40); // 同步 Pascal 的 sdl_delay(20)
    }

    // 动画播放完后，加载 Background.Pic 的 Index 1 作为开始菜单背景
    if (m_texBeginBackground) {
        SDL_DestroyTexture(m_texBeginBackground);
    }
    
    PicImage bgPic = PicLoader::loadPic("resource/Background.Pic", 0);
    if (bgPic.surface) {
        m_texBeginBackground = SDL_CreateTextureFromSurface(m_renderer, bgPic.surface);
        PicLoader::freePic(bgPic);
    }
}

void UIManager::DrawTitleScreen() {
    // 绘制开始菜单画面
    DrawTitleBackground();
    // 可以在此处添加按钮绘制逻辑，对应 Pascal 的 drawtitlepic(0, x, y)
}

void UIManager::DrawTitleBackground() {
    if (m_texBeginBackground) {
        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        SDL_FRect dest = { 0.0f, 0.0f, (float)w, (float)h };
        SDL_RenderTexture(m_renderer, m_texBeginBackground, NULL, &dest);
    }
}
void UIManager::DrawCenteredTexture(SDL_Texture* tex) {
    if (!tex) return;
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    float tw, th;
    SDL_GetTextureSize(tex, &tw, &th);
    
    SDL_FRect dest = { (float)(w - tw) / 2, (float)(h - th) / 2, tw, th };
    SDL_RenderTexture(m_renderer, tex, NULL, &dest);
}

void UIManager::DrawText(const std::string& text, int x, int y, uint32_t color, int fontSize) {
    // Assumes GBK input, uses TextManager to convert and render
    TextManager::getInstance().RenderText(text, x, y, color);
}

void UIManager::DrawTextUtf8(const std::string& text, int x, int y, uint32_t color, int fontSize) {
    TextManager::getInstance().RenderTextUtf8(text, x, y, color, fontSize);
}

void UIManager::DrawShadowText(const std::string& text, int x, int y, uint32_t color1, uint32_t color2, int fontSize) {
    // Shadow
    TextManager::getInstance().RenderText(text, x + 1, y + 1, color2);
    // Main
    TextManager::getInstance().RenderText(text, x, y, color1);
}

void UIManager::ShowCharacterCreation(const Role& role) {
    if (m_texBeginBackground) {
        DrawTitleBackground();
    }

    DrawRectangle(100, 100, 440, 200, 0, 0xFFFFFFFF, 200);
    if (m_font) {
        const int wrapWidth = 420;
        SDL_Color shadow = { 0, 0, 0, 255 };
        SDL_Color main = { 255, 255, 255, 255 };
        SDL_Surface* s2 = TTF_RenderText_Solid_Wrapped(m_font, "請輸入主角姓名: (Enter 確認 / Backspace 刪除 / R 重骰 / Esc 返回)", 0, shadow, wrapWidth);
        if (s2) {
            SDL_Texture* t2 = SDL_CreateTextureFromSurface(m_renderer, s2);
            SDL_FRect d2 = { 121.0f, 141.0f, (float)s2->w, (float)s2->h };
            SDL_RenderTexture(m_renderer, t2, NULL, &d2);
            SDL_DestroyTexture(t2);
            SDL_DestroySurface(s2);
        }

        SDL_Surface* s1 = TTF_RenderText_Solid_Wrapped(m_font, "請輸入主角姓名: (Enter 確認 / Backspace 刪除 / R 重骰 / Esc 返回)", 0, main, wrapWidth);
        if (s1) {
            SDL_Texture* t1 = SDL_CreateTextureFromSurface(m_renderer, s1);
            SDL_FRect d1 = { 120.0f, 140.0f, (float)s1->w, (float)s1->h };
            SDL_RenderTexture(m_renderer, t1, NULL, &d1);
            SDL_DestroyTexture(t1);
            SDL_DestroySurface(s1);
        }
    }
    DrawShadowTextUtf8(TextManager::getInstance().nameToUtf8(role.getName()), 280, 200, 0xFFFF00FF, 0x000000FF);
}

void UIManager::ShowSaveLoadMenu(bool isSave) {
    bool running = true;
    int currentSelection = 0;
    SDL_Event event;
    const int SLOT_COUNT = 3;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                } else if (event.key.key == SDLK_DOWN || event.key.key == SDLK_KP_2) {
                    currentSelection = (currentSelection + 1) % SLOT_COUNT;
                } else if (event.key.key == SDLK_UP || event.key.key == SDLK_KP_8) {
                    currentSelection = (currentSelection + SLOT_COUNT - 1) % SLOT_COUNT;
                } else if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                    if (isSave) {
                        GameManager::getInstance().SaveGame(currentSelection + 1);
                        ShowDialogue("進度已保存", 0, 0); 
                    } else {
                        GameManager::getInstance().LoadGame(currentSelection + 1);
                        running = false; // Exit menu after load
                    }
                }
            }
        }

        SDL_RenderClear(m_renderer);
        if (m_texMenuBackground) {
            SDL_RenderTexture(m_renderer, m_texMenuBackground, NULL, NULL);
        } else {
            // If no background captured, use default grey
            SDL_SetRenderDrawColor(m_renderer, 50, 50, 50, 255);
            SDL_RenderClear(m_renderer);
        }

        DrawRectangle(150, 50, 340, 300, 0, 0xFFFFFFFF, 100);
        DrawShadowTextUtf8(isSave ? "保存進度" : "讀取進度", 280, 60, 0xFFFFFFFF, 0x000000FF);

        for (int i = 0; i < SLOT_COUNT; ++i) {
            uint32_t color = (i == currentSelection) ? 0xFFFF00FF : 0xFFFFFFFF;
            std::string slotName = "進度 " + std::to_string(i + 1);
            // Check if file exists (Stub logic, assuming exists for display)
            // In real impl, check file existence.
            DrawShadowTextUtf8(slotName, 200, 100 + i * 50, color, 0x000000FF);
        }

        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
}

void UIManager::ShowDialogue(const std::string& text, int headId, int mode, const std::string& nameUtf8, const std::string& nameRawBytes) {
    SDL_Event event;
    static bool s_showNameDebug = false;

    auto bytesToHex = [](const std::string& s, size_t maxBytes) -> std::string {
        static const char* kHex = "0123456789ABCDEF";
        std::string out;
        size_t n = std::min(maxBytes, s.size());
        out.reserve(n * 3 + 8);
        for (size_t i = 0; i < n; ++i) {
            unsigned char b = static_cast<unsigned char>(s[i]);
            out.push_back(kHex[(b >> 4) & 0xF]);
            out.push_back(kHex[b & 0xF]);
            if (i + 1 < n) out.push_back(' ');
        }
        if (s.size() > maxBytes) out += " ..";
        return out;
    };

    auto hasReplacement = [](const std::string& s) -> bool {
        for (size_t i = 0; i + 2 < s.size(); ++i) {
            if (static_cast<unsigned char>(s[i]) == 0xEF &&
                static_cast<unsigned char>(s[i + 1]) == 0xBF &&
                static_cast<unsigned char>(s[i + 2]) == 0xBD) {
                return true;
            }
        }
        return false;
    };

    auto sanitizeLabel = [](std::string s) -> std::string {
        while (!s.empty()) {
            unsigned char b = static_cast<unsigned char>(s.back());
            if (b == 0 || b <= 0x20 || b == 0x7F) s.pop_back();
            else break;
        }
        size_t zeroPos = s.find('\0');
        if (zeroPos != std::string::npos) s.resize(zeroPos);
        return s;
    };

    auto takePageText = [&](const std::string& remaining, int wrapWidth, int maxTextHeight) -> std::string {
        if (!m_font || wrapWidth <= 10 || maxTextHeight <= 5) return remaining;
        if (remaining.empty()) return remaining;

        std::vector<size_t> ends;
        ends.reserve(remaining.size());
        size_t i = 0;
        while (i < remaining.size()) {
            size_t j = i + 1;
            while (j < remaining.size() && (static_cast<unsigned char>(remaining[j]) & 0xC0) == 0x80) {
                j++;
            }
            ends.push_back(j);
            i = j;
        }

        auto fits = [&](size_t endIndex) -> bool {
            SDL_Color c = { 255, 255, 255, 255 };
            std::string candidate = remaining.substr(0, endIndex);
            SDL_Surface* s = TTF_RenderText_Solid_Wrapped(m_font, candidate.c_str(), 0, c, wrapWidth);
            if (!s) return true;
            bool ok = s->h <= maxTextHeight;
            SDL_DestroySurface(s);
            return ok;
        };

        size_t lo = 0;
        size_t hi = ends.size();
        size_t best = 0;
        while (lo < hi) {
            size_t mid = (lo + hi) / 2;
            size_t endIndex = ends[mid];
            if (fits(endIndex)) {
                best = endIndex;
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        if (best == 0) best = ends.front();
        return remaining.substr(0, best);
    };

    std::string showName = sanitizeLabel(nameUtf8);
    std::string remainingText = text;
    std::string rawNameBytes = nameRawBytes;
    SDL_Texture* frozenBackground = nullptr;
    SDL_Surface* screenSurface = GameManager::getInstance().getScreenSurface();
    if (screenSurface) {
        frozenBackground = SDL_CreateTextureFromSurface(m_renderer, screenSurface);
    }

    while (true) {
        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);

        int boxH = 150;
        int boxY = h - boxH - 20;
        const int textX = (headId >= 0) ? 150 : 40;
        const int textY = boxY + 40;
        const int wrapWidth = (w - 20) - textX;
        const int maxTextH = (boxY + boxH - 10) - textY;

        std::string pageText = sanitizeLabel(takePageText(remainingText, wrapWidth, maxTextH));
        if (pageText.empty() && !remainingText.empty()) {
            pageText = remainingText.substr(0, 1);
        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return;
            }
        }

        bool waiting = true;
        while (waiting) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_EVENT_QUIT) {
                    GameManager::getInstance().Quit();
                    return;
                }
                if (event.type == SDL_EVENT_KEY_DOWN) {
                    if (event.key.repeat) continue;
                    if (event.key.key == SDLK_F3) {
                        s_showNameDebug = !s_showNameDebug;
                    }
                    if (event.key.key == SDLK_SPACE || event.key.key == SDLK_RETURN || event.key.key == SDLK_ESCAPE) {
                        waiting = false;
                    }
                }
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    waiting = false;
                }
            }

            SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
            SDL_RenderClear(m_renderer);
            if (frozenBackground) {
                SDL_RenderTexture(m_renderer, frozenBackground, NULL, NULL);
            } else {
                GameManager::getInstance().RenderScreenTo(m_renderer);
            }

            DrawRectangle(20, boxY, w - 40, boxH, 0x000000CC, 0xFFFFFFFF, 200);

            if (headId >= 0) {
                DrawHead(headId, 40, boxY + 25);
            }

            if (!showName.empty()) {
                int nameX = 40;
                int nameY = boxY + 5;
                if (headId >= 0) {
                    const int headY = boxY + 25;
                    const int approxHeadH = 64;
                    nameY = headY + approxHeadH + 6;
                    const int maxNameY = boxY + boxH - 24;
                    if (nameY > maxNameY) nameY = boxY + 5;
                }
                DrawShadowTextUtf8(showName, nameX, nameY, 0xFFFF00FF, 0x000000FF);
            }
            if (s_showNameDebug && headId >= 0) {
                if (rawNameBytes.empty()) rawNameBytes = showName;
                std::string d1 = std::string("FONT: ") + (g_loadedFontPath.empty() ? "<none>" : g_loadedFontPath);
                std::string d2 = std::string("RAW(") + std::to_string(rawNameBytes.size()) + "): " + bytesToHex(rawNameBytes, 24);
                std::string d3 = std::string("UTF8(") + std::to_string(showName.size()) + ") repl=" + (hasReplacement(showName) ? "1" : "0") + ": " + bytesToHex(showName, 24);
                DrawShadowTextUtf8(d1, 40, boxY + 22, 0xFFFFFFFF, 0x000000FF);
                DrawShadowTextUtf8(d2, 40, boxY + 38, 0xFFFFFFFF, 0x000000FF);
                DrawShadowTextUtf8(d3, 40, boxY + 54, 0xFFFFFFFF, 0x000000FF);
            }

            if (m_font && wrapWidth > 10 && !pageText.empty()) {
                SDL_Color shadow = { 0, 0, 0, 255 };
                SDL_Color main = { 255, 255, 255, 255 };

                SDL_Surface* s2 = TTF_RenderText_Solid_Wrapped(m_font, pageText.c_str(), 0, shadow, wrapWidth);
                if (s2) {
                    SDL_Texture* t2 = SDL_CreateTextureFromSurface(m_renderer, s2);
                    SDL_FRect d2 = { (float)(textX + 1), (float)(textY + 1), (float)s2->w, (float)s2->h };
                    SDL_RenderTexture(m_renderer, t2, NULL, &d2);
                    SDL_DestroyTexture(t2);
                    SDL_DestroySurface(s2);
                }

                SDL_Surface* s1 = TTF_RenderText_Solid_Wrapped(m_font, pageText.c_str(), 0, main, wrapWidth);
                if (s1) {
                    SDL_Texture* t1 = SDL_CreateTextureFromSurface(m_renderer, s1);
                    SDL_FRect d1 = { (float)textX, (float)textY, (float)s1->w, (float)s1->h };
                    SDL_RenderTexture(m_renderer, t1, NULL, &d1);
                    SDL_DestroyTexture(t1);
                    SDL_DestroySurface(s1);
                }
            }

            SDL_RenderPresent(m_renderer);
            SDL_Delay(16);
        }

        if (pageText.size() >= remainingText.size()) break;
        remainingText.erase(0, pageText.size());
        while (!remainingText.empty() && (remainingText[0] == '\n' || remainingText[0] == '\r')) {
            remainingText.erase(0, 1);
        }
    }
    if (frozenBackground) {
        SDL_DestroyTexture(frozenBackground);
    }
}

void UIManager::ShowTitle(const std::string& text, int x, int y, uint32_t color1, uint32_t color2) {
    DrawShadowText(text, x, y, color1, color2, 40); // Larger font for title
}

int UIManager::ShowChoice(const std::string& text) {
    // Parse choices from text (e.g., "Yes/No" or similar?)
    // Pascal often passes options in a specific way or just uses a menu.
    // Assuming simple Yes/No for now or single choice.
    // Or maybe the text is the prompt, and we provide standard choices?
    // Let's implement a simple Yes/No dialog.
    
    bool running = true;
    int selection = 0; // 0=Yes, 1=No
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
             if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                return 0;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_LEFT || event.key.key == SDLK_RIGHT) {
                    selection = !selection;
                }
                if (event.key.key == SDLK_RETURN || event.key.key == SDLK_SPACE) {
                    return (selection == 0) ? 1 : 0; // 1 for Yes
                }
            }
        }

        GameManager::getInstance().RenderScreenTo(m_renderer);
        
        // Draw Box
        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        DrawRectangle(w/2 - 150, h/2 - 60, 300, 120, 0x000000CC, 0xFFFFFFFF, 200);
        
        DrawText(text, w/2 - 100, h/2 - 40, 0xFFFFFFFF);

        // Draw Options
        uint32_t colYes = (selection == 0) ? 0xFFFF00FF : 0xFFFFFFFF;
        uint32_t colNo = (selection == 1) ? 0xFFFF00FF : 0xFFFFFFFF;
        
        DrawShadowTextUtf8("是 (Yes)", w/2 - 80, h/2 + 10, colYes, 0x000000FF);
        DrawShadowTextUtf8("否 (No)", w/2 + 20, h/2 + 10, colNo, 0x000000FF);

        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }
    return 0;
}

void UIManager::ShowItemNotification(int itemId, int amount) {
    Item& item = GameManager::getInstance().getItem(itemId);
    std::string nameUtf8 = TextManager::getInstance().gbkToUtf8(item.getName());
    bool gain = amount >= 0;
    int showAmount = std::abs(amount);
    std::string title = gain ? " 得到物品" : " 失去物品";

    PicImage pic = PicLoader::loadPic("resource/Items.Pic", itemId);
    SDL_Texture* itemTex = nullptr;
    int picW = 0;
    int picH = 0;
    if (pic.surface) {
        itemTex = SDL_CreateTextureFromSurface(m_renderer, pic.surface);
        if (itemTex) {
            SDL_SetTextureBlendMode(itemTex, SDL_BLENDMODE_BLEND);
            picW = pic.surface->w;
            picH = pic.surface->h;
        }
    }

    SDL_Texture* frozenBackground = nullptr;
    SDL_Surface* screenSurface = GameManager::getInstance().getScreenSurface();
    if (screenSurface) {
        frozenBackground = SDL_CreateTextureFromSurface(m_renderer, screenSurface);
    }

    bool waiting = true;
    SDL_Event event;
    while (waiting) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                GameManager::getInstance().Quit();
                waiting = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                waiting = false;
            }
        }

        int w, h;
        SDL_GetWindowSize(m_window, &w, &h);
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, 255);
        SDL_RenderClear(m_renderer);
        if (frozenBackground) {
            SDL_RenderTexture(m_renderer, frozenBackground, NULL, NULL);
        } else {
            GameManager::getInstance().RenderScreenTo(m_renderer);
        }

        int boxW = 260;
        int boxH = 140;
        int boxX = (w - boxW) / 2;
        int boxY = (h - boxH) / 2;
        DrawRectangle(boxX, boxY, boxW, boxH, 0x000000CC, 0xFFFFFFFF, 200);

        DrawShadowTextUtf8(title, boxX + 12, boxY + 10, 0xFFFF00FF, 0x000000FF);
        if (itemTex && picW > 0 && picH > 0) {
            SDL_FRect dest = { (float)(boxX + (boxW - picW) / 2), (float)(boxY + 35), (float)picW, (float)picH };
            SDL_RenderTexture(m_renderer, itemTex, NULL, &dest);
        }
        DrawShadowTextUtf8(nameUtf8, boxX + 12, boxY + 35 + picH + 6, 0xFFFFFFFF, 0x000000FF);
        DrawShadowTextUtf8(" 數量", boxX + 12, boxY + 35 + picH + 30, 0xFFFF00FF, 0x000000FF);
        DrawShadowTextUtf8(std::to_string(showAmount), boxX + 70, boxY + 35 + picH + 30, 0xFFFFFFFF, 0x000000FF);

        SDL_RenderPresent(m_renderer);
        SDL_Delay(16);
    }

    if (frozenBackground) SDL_DestroyTexture(frozenBackground);
    if (itemTex) SDL_DestroyTexture(itemTex);
    if (pic.surface) PicLoader::freePic(pic);
}

void UIManager::FadeScreen(bool fadeIn) {
    // fadeIn: Black -> Transparent
    // !fadeIn: Transparent -> Black
    
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    
    int steps = 20;
    for (int i = 0; i <= steps; ++i) {
        int alpha = fadeIn ? (255 - i * 255 / steps) : (i * 255 / steps);
        
        GameManager::getInstance().RenderScreenTo(m_renderer);
        
        SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer, 0, 0, 0, alpha);
        SDL_FRect rect = { 0.0f, 0.0f, (float)w, (float)h };
        SDL_RenderFillRect(m_renderer, &rect);
        
        SDL_RenderPresent(m_renderer);
        SDL_Delay(20);
    }
}

void UIManager::FlashScreen(uint32_t color, int durationMs) {
    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    
    // Draw colored rect
    GameManager::getInstance().RenderScreenTo(m_renderer);
    
    SDL_SetRenderDrawBlendMode(m_renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer, (color >> 24) & 0xFF, (color >> 16) & 0xFF, (color >> 8) & 0xFF, 128);
    SDL_FRect rect = { 0.0f, 0.0f, (float)w, (float)h };
    SDL_RenderFillRect(m_renderer, &rect);
    
    SDL_RenderPresent(m_renderer);
    SDL_Delay(durationMs);
    
    // Clear effect
    GameManager::getInstance().RenderScreenTo(m_renderer);
    SDL_RenderPresent(m_renderer);
}

void UIManager::UpdateScreen() {
    SDL_RenderPresent(m_renderer);
}
