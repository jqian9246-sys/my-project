#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <ctime>
#include <conio.h>
#include <windows.h>

using namespace std;

const int BOARD_SIZE = 10;
const int MINES_EACH = 10;     // 每人雷数
const int HPOT_EACH = 1;       // 每人血瓶数
const int INIT_HP = 10;        // 初始血量
const int DIRECT_DMG = 5;      // 踩雷直接伤害(每雷)
const int SPLASH_DMG = 2;      // 溅射伤害(每雷)
const int HEAL_AMT = 5;        // 血瓶回复(每瓶)

// 位掩码
const int MINE_A = 1;   // bit0
const int MINE_B = 2;   // bit1
const int HPOT_A = 4;   // bit2
const int HPOT_B = 8;   // bit3

#define COLOR_DEFAULT  7
#define COLOR_RED      12
#define COLOR_BLUE     9
#define COLOR_GREEN    10
#define COLOR_YELLOW   14
#define COLOR_CYAN     11
#define COLOR_WHITE    15
#define COLOR_MAGENTA  13

void setColor(int color) {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(h, color);
}
void clearScreen() { system("cls"); }

// ===== 音效函数 =====
void sfxPlace()      { Beep(600, 50); }                    // 布雷/血瓶：中音短促
void sfxMove()       { Beep(400, 30); }                    // 移动：短促低音
void sfxHeal()       { Beep(800, 80); Beep(1000, 80); }     // 血瓶：上升双音
void sfxExplode()    { Beep(100, 80); Beep(300, 80); Beep(100, 80);
                        Beep(300, 80); Beep(100, 200); }    // 爆炸：多音阶
void sfxGameStart()  { Beep(523, 100); Beep(659, 100);      // 开场：C5-E5-G5 上行
                        Beep(784, 200); }
void sfxWin()        { Beep(784, 120); Beep(988, 120);      // 胜利：G5-B5-C6
                        Beep(1048, 300); }
void sfxLose()       { Beep(300, 200); Beep(200, 400); }    // 失败：低沉下行
void sfxDraw()       { Beep(440, 150); Beep(440, 150); }    // 平局：双响中音

// ===================== Board =====================
class Board {
private:
    vector< vector<int> > cell;      // 位掩码
    vector< vector<bool> > revealed;

public:
    Board() {
        cell.assign(BOARD_SIZE, vector<int>(BOARD_SIZE, 0));
        revealed.assign(BOARD_SIZE, vector<bool>(BOARD_SIZE, false));
    }

    // ===== 基础操作 =====
    int getMask(int r, int c) { return cell[r][c]; }

    void setMask(int r, int c, int mask) { cell[r][c] = mask; }

    void addMask(int r, int c, int mask) { cell[r][c] |= mask; }

    void clearMask(int r, int c, int mask) { cell[r][c] &= ~mask; }

    bool hasMask(int r, int c, int mask) { return (cell[r][c] & mask) != 0; }

    bool hasAnyMine(int r, int c) { return (cell[r][c] & (MINE_A | MINE_B)) != 0; }

    int mineCount(int r, int c) {
        int cnt = 0;
        if (cell[r][c] & MINE_A) cnt++;
        if (cell[r][c] & MINE_B) cnt++;
        return cnt;
    }

    int hpotCount(int r, int c) {
        int cnt = 0;
        if (cell[r][c] & HPOT_A) cnt++;
        if (cell[r][c] & HPOT_B) cnt++;
        return cnt;
    }

    bool isRevealed(int r, int c) { return revealed[r][c]; }

    void reveal(int r, int c) { revealed[r][c] = true; }

    void revealAll() {
        for (int r = 0; r < BOARD_SIZE; r++)
            for (int c = 0; c < BOARD_SIZE; c++)
                revealed[r][c] = true;
    }

    bool hasPlayerMine(int r, int c, int player) {
        return (cell[r][c] & (player == 1 ? MINE_A : MINE_B)) != 0;
    }

    bool hasPlayerHpot(int r, int c, int player) {
        return (cell[r][c] & (player == 1 ? HPOT_A : HPOT_B)) != 0;
    }

    // 获取所有未掀开格子
    vector< pair<int, int> > getUnrevealedCells() {
        vector< pair<int, int> > v;
        for (int r = 0; r < BOARD_SIZE; r++)
            for (int c = 0; c < BOARD_SIZE; c++)
                if (!revealed[r][c]) v.push_back(make_pair(r, c));
        return v;
    }

    // 是否全部非雷格子都掀开了
    bool allSafeRevealed() {
        for (int r = 0; r < BOARD_SIZE; r++)
            for (int c = 0; c < BOARD_SIZE; c++)
                if (!revealed[r][c] && !hasAnyMine(r, c) && hpotCount(r, c) == 0)
                    return false;
        return true;
    }

    // ===== 布雷阶段打印 =====
    void printForPlacement(int curRow, int curCol, int player) {
        setColor(COLOR_WHITE);
        cout << "  ";
        for (int c = 0; c < BOARD_SIZE; c++) cout << " " << (char)('A' + c) << " ";
        cout << endl;
        for (int r = 0; r < BOARD_SIZE; r++) {
            setColor(COLOR_WHITE);
            if (r < 9) cout << " "; cout << (r + 1) << " ";
            for (int c = 0; c < BOARD_SIZE; c++) {
                bool isCur = (r == curRow && c == curCol);
                int m = cell[r][c];
                bool myMine = (m & (player == 1 ? MINE_A : MINE_B)) != 0;
                bool myHpot = (m & (player == 1 ? HPOT_A : HPOT_B)) != 0;

                if (myMine && myHpot) {
                    setColor(player == 1 ? COLOR_RED : COLOR_BLUE);
                    cout << "[M+]";
                } else if (myMine) {
                    setColor(player == 1 ? COLOR_RED : COLOR_BLUE);
                    cout << (player == 1 ? "[*]" : "[@]");
                } else if (myHpot) {
                    setColor(COLOR_GREEN);
                    cout << "[+]";
                } else if (isCur) {
                    setColor(COLOR_CYAN); cout << "[ ]";
                } else {
                    setColor(COLOR_DEFAULT); cout << " . ";
                }
            }
            cout << endl;
        }
        setColor(COLOR_DEFAULT);
    }

    // ===== 游戏阶段打印 =====
    void printForGame(int p1Row, int p1Col, int p2Row, int p2Col) {
        setColor(COLOR_WHITE);
        cout << "  ";
        for (int c = 0; c < BOARD_SIZE; c++) cout << " " << (char)('A' + c) << " ";
        cout << endl;
        for (int r = 0; r < BOARD_SIZE; r++) {
            setColor(COLOR_WHITE);
            if (r < 9) cout << " "; cout << (r + 1) << " ";

            for (int c = 0; c < BOARD_SIZE; c++) {
                bool p1Here = (r == p1Row && c == p1Col);
                bool p2Here = (r == p2Row && c == p2Col);
                bool both = p1Here && p2Here;
                // 玩家标记优先
                if (both) {
                    setColor(COLOR_MAGENTA); cout << "[AB]";
                } else if (p1Here) {
                    setColor(COLOR_RED); cout << "[A]";
                } else if (p2Here) {
                    setColor(COLOR_BLUE); cout << "[B]";
                } else if (!revealed[r][c]) {
                    setColor(COLOR_DEFAULT); cout << "[#]";
                } else {
                    // 已掀开：显示内容
                    int m = cell[r][c];
                    bool mA = (m & MINE_A) != 0;
                    bool mB = (m & MINE_B) != 0;
                    bool hA = (m & HPOT_A) != 0;
                    bool hB = (m & HPOT_B) != 0;

                    if (mA && mB) {
                        setColor(COLOR_YELLOW); cout << "[!!]";
                    } else if (mA) {
                        if (hA || hB) { setColor(COLOR_MAGENTA); cout << "[*+]"; }
                        else { setColor(COLOR_RED); cout << "[*]"; }
                    } else if (mB) {
                        if (hA || hB) { setColor(COLOR_MAGENTA); cout << "[@+]"; }
                        else { setColor(COLOR_BLUE); cout << "[@]"; }
                    } else if (hA || hB) {
                        setColor(COLOR_GREEN); cout << "[+]";
                    } else {
                        // 空地：显示周围总雷数（双方叠加）
                        int nearby = mineCount(r, c);
                        for (int dr = -1; dr <= 1; dr++)
                            for (int dc = -1; dc <= 1; dc++) {
                                int nr = r + dr, nc = c + dc;
                                if (nr >= 0 && nr < BOARD_SIZE && nc >= 0 && nc < BOARD_SIZE)
                                    nearby += mineCount(nr, nc);
                            }
                        if (nearby == 0) {
                            setColor(COLOR_DEFAULT); cout << "   ";
                        } else {
                            setColor(COLOR_GREEN); cout << " " << nearby << " ";
                        }
                    }
                }
            }
            cout << endl;
        }
        setColor(COLOR_DEFAULT);
    }

    // ===== 终局打印 =====
    void printFinal() {
        setColor(COLOR_WHITE);
        cout << "  ";
        for (int c = 0; c < BOARD_SIZE; c++) cout << " " << (char)('A' + c) << " ";
        cout << endl;
        for (int r = 0; r < BOARD_SIZE; r++) {
            setColor(COLOR_WHITE);
            if (r < 9) cout << " "; cout << (r + 1) << " ";
            for (int c = 0; c < BOARD_SIZE; c++) {
                int m = cell[r][c];
                bool mA = (m & MINE_A) != 0, mB = (m & MINE_B) != 0;
                bool hA = (m & HPOT_A) != 0, hB = (m & HPOT_B) != 0;
                if (mA && mB) { setColor(COLOR_YELLOW); cout << "[!!]"; }
                else if (mA) {
                    if (hA || hB) { setColor(COLOR_MAGENTA); cout << "[*+]"; }
                    else { setColor(COLOR_RED); cout << "[*]"; }
                }
                else if (mB) {
                    if (hA || hB) { setColor(COLOR_MAGENTA); cout << "[@+]"; }
                    else { setColor(COLOR_BLUE); cout << "[@]"; }
                }
                else if (hA || hB) { setColor(COLOR_GREEN); cout << "[+]"; }
                else { setColor(COLOR_DEFAULT); cout << "   "; }
            }
            cout << endl;
        }
        setColor(COLOR_DEFAULT);
    }
};

// ===================== 解析输入 =====================
bool parseInput(const string& input, int& row, int& col) {
    if (input.length() < 2) return false;
    char colChar = toupper(input[0]);
    if (colChar < 'A' || colChar > 'A' + BOARD_SIZE - 1) {
        char lastChar = toupper(input[input.length() - 1]);
        if (lastChar >= 'A' && lastChar <= 'A' + BOARD_SIZE - 1) {
            colChar = lastChar;
            string rowStr = input.substr(0, input.length() - 1);
            row = atoi(rowStr.c_str()) - 1; col = colChar - 'A';
        } else return false;
    } else {
        col = colChar - 'A';
        row = atoi(input.substr(1).c_str()) - 1;
    }
    return row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE;
}

// 布雷阶段（玩家手动）
void placementPhase(Board& board, int player, const string& playerName, int minesLeft, int hpotsLeft) {
    vector< pair<int, int> > placedMines, placedHpots;
    int curRow = 0, curCol = 0;

    while ((int)placedMines.size() < minesLeft || (int)placedHpots.size() < hpotsLeft) {
        clearScreen();
        setColor(player == 1 ? COLOR_RED : COLOR_BLUE);
        cout << "========================================" << endl;
        cout << "  " << playerName << " 布雷阶段" << endl;
        cout << "  地雷: " << placedMines.size() << "/" << minesLeft
             << "  血瓶: " << placedHpots.size() << "/" << hpotsLeft << endl;
        cout << "========================================" << endl;
        setColor(COLOR_YELLOW);
        cout << "操作: M-布地雷  H-布血瓶  R-取消  W/A/S/D-移动  Q-完成" << endl;
        cout << "光标: " << (char)('A' + curCol) << (curRow + 1) << endl;
        setColor(COLOR_DEFAULT); cout << endl;

        board.printForPlacement(curRow, curCol, player);

        cout << endl;
        setColor(COLOR_CYAN); cout << ">> ";
        char cmd = toupper(_getch()); cout << cmd << endl;

        int myMineBit = (player == 1) ? MINE_A : MINE_B;
        int myHpotBit = (player == 1) ? HPOT_A : HPOT_B;

        if (cmd == 'W' && curRow > 0) curRow--;
        else if (cmd == 'S' && curRow < BOARD_SIZE - 1) curRow++;
        else if (cmd == 'A' && curCol > 0) curCol--;
        else if (cmd == 'D' && curCol < BOARD_SIZE - 1) curCol++;
        else if (cmd == 'M') {
            if ((int)placedMines.size() >= minesLeft) {
                setColor(COLOR_YELLOW); cout << "地雷已布置完毕！"; _getch();
            } else if (board.hasMask(curRow, curCol, myMineBit)) {
                setColor(COLOR_YELLOW); cout << "该位置已布置了你的地雷！"; _getch();
            } else {
                board.addMask(curRow, curCol, myMineBit);
                placedMines.push_back(make_pair(curRow, curCol));
                sfxPlace();
            }
        } else if (cmd == 'H') {
            if ((int)placedHpots.size() >= hpotsLeft) {
                setColor(COLOR_YELLOW); cout << "血瓶已布置完毕！"; _getch();
            } else if (board.hasMask(curRow, curCol, myHpotBit)) {
                setColor(COLOR_YELLOW); cout << "该位置已布置了你的血瓶！"; _getch();
            } else {
                board.addMask(curRow, curCol, myHpotBit);
                placedHpots.push_back(make_pair(curRow, curCol));
                sfxPlace();
            }
        } else if (cmd == 'R') {
            // 取消当前位置自己的物品
            if (board.hasMask(curRow, curCol, myMineBit)) {
                board.clearMask(curRow, curCol, myMineBit);
                for (size_t i = 0; i < placedMines.size(); i++)
                    if (placedMines[i].first == curRow && placedMines[i].second == curCol)
                    { placedMines.erase(placedMines.begin() + i); break; }
            } else if (board.hasMask(curRow, curCol, myHpotBit)) {
                board.clearMask(curRow, curCol, myHpotBit);
                for (size_t i = 0; i < placedHpots.size(); i++)
                    if (placedHpots[i].first == curRow && placedHpots[i].second == curCol)
                    { placedHpots.erase(placedHpots.begin() + i); break; }
            } else {
                setColor(COLOR_YELLOW); cout << "该位置没有你的物品！"; _getch();
            }
        } else if (cmd == 'Q' && (int)placedMines.size() == minesLeft && (int)placedHpots.size() == hpotsLeft) {
            break;
        } else if (cmd == 'Q') {
            setColor(COLOR_YELLOW);
            cout << "请完成所有布置！（地雷" << minesLeft << "个 血瓶" << hpotsLeft << "个）";
            _getch();
        }
    }

    clearScreen();
    setColor(COLOR_GREEN);
    cout << playerName << " 布置完成！地雷" << placedMines.size() << "个 血瓶" << placedHpots.size() << "个" << endl;
    cout << "按任意键继续...";
    _getch();
}

// AI 自动布雷
void aiPlaceItems(Board& board, int player, int mines, int hpots) {
    int myMineBit = (player == 1) ? MINE_A : MINE_B;
    int myHpotBit = (player == 1) ? HPOT_A : HPOT_B;

    int placed = 0;
    while (placed < mines) {
        int r = rand() % BOARD_SIZE, c = rand() % BOARD_SIZE;
        if (!board.hasMask(r, c, myMineBit)) {
            board.addMask(r, c, myMineBit); placed++;
        }
    }
    placed = 0;
    while (placed < hpots) {
        int r = rand() % BOARD_SIZE, c = rand() % BOARD_SIZE;
        if (!board.hasMask(r, c, myHpotBit)) {
            board.addMask(r, c, myHpotBit); placed++;
        }
    }
}

// ===================== 引爆逻辑 =====================
// 返回值: 对踩中者造成的直接伤害（已在函数外处理血瓶回复）
struct ExplosionResult {
    int directDmg;       // 踩中者受到的直接伤害
    int splashDmgA;      // A受到的溅射伤害
    int splashDmgB;      // B受到的溅射伤害
    int heal;            // 踩中者回复血量
    vector<string> log;  // 日志
};

ExplosionResult processCell(Board& board, int row, int col, int currentPlayer,
                             int posR_A, int posC_A, int posR_B, int posC_B) {
    ExplosionResult res = {0, 0, 0, 0, {}};

    // === 血瓶回复（先回血再算伤害） ===
    int hpots = board.hpotCount(row, col);
    if (hpots > 0) {
        sfxHeal();
        res.heal = hpots * HEAL_AMT;
        board.clearMask(row, col, HPOT_A | HPOT_B);
        res.log.push_back("捡到 " + to_string(hpots) + " 个血瓶，回复 " + to_string(res.heal) + " 点血量！");
    }

    // === 地雷直接伤害 ===
    int minesHere = board.mineCount(row, col);
    if (minesHere > 0) {
        sfxExplode();
        res.directDmg = minesHere * DIRECT_DMG;
        char buf[100];
        sprintf(buf, "踩中 %d 颗地雷，受到 %d 点直接伤害！", minesHere, res.directDmg);
        res.log.push_back(buf);

        // 清除当前格的地雷
        board.clearMask(row, col, MINE_A | MINE_B);

        // === 9宫格溅射：不引爆雷，只对范围内的玩家造成溅射伤害 ===
        for (int dr = -1; dr <= 1; dr++) {
            for (int dc = -1; dc <= 1; dc++) {
                int r = row + dr, c = col + dc;
                if (r < 0 || r >= BOARD_SIZE || c < 0 || c >= BOARD_SIZE) continue;
                if (dr == 0 && dc == 0) continue;  // 中心格已处理直接伤害

                int m = board.getMask(r, c);
                int minesNearby = board.mineCount(r, c);
                if (minesNearby == 0) continue;

                // 检查A是否在此格
                if (posR_A == r && posC_A == c) {
                    res.splashDmgA += minesNearby * SPLASH_DMG;
                }
                // 检查B是否在此格
                if (posR_B == r && posC_B == c) {
                    res.splashDmgB += minesNearby * SPLASH_DMG;
                }
            }
        }

        if (res.splashDmgA > 0)
            res.log.push_back("溅射波及A所在格的雷，A受到 " + to_string(res.splashDmgA) + " 点溅射伤害！");
        if (res.splashDmgB > 0)
            res.log.push_back("溅射波及B所在格的雷，B受到 " + to_string(res.splashDmgB) + " 点溅射伤害！");
    }

    board.reveal(row, col);
    return res;
}

// ===================== 游戏主阶段 =====================
void gamePlayPhase(Board& board, const string& nameA, const string& nameB, bool isSingle) {
    // 玩家位置：初始 (-1,-1)表示未放置，开局时可选择初始站位
    int posR[3] = {-1, -1, -1};  // 1=A, 2=B
    int posC[3] = {-1, -1, -1};
    int hp[3] = {0, INIT_HP, INIT_HP};

    int cur = 1;  // A先手
    bool over = false;

    // 初始站位阶段（第一回合选位置，不扣血/不触发效果）
    bool firstMove[3] = {false, true, true};

    while (!over) {
        clearScreen();

        // 血量条
        setColor(COLOR_RED);
        cout << nameA << " HP: [";
        int barsA = hp[1] * 2;
        for (int i = 0; i < 20; i++) cout << (i < barsA ? '#' : ' ');
        cout << "] " << hp[1] << "/" << INIT_HP << endl;

        setColor(COLOR_BLUE);
        cout << nameB << " HP: [";
        int barsB = hp[2] * 2;
        for (int i = 0; i < 20; i++) cout << (i < barsB ? '#' : ' ');
        cout << "] " << hp[2] << "/" << INIT_HP;
        if (isSingle) { setColor(COLOR_CYAN); cout << " (AI)"; }
        cout << endl;

        setColor(cur == 1 ? COLOR_RED : COLOR_BLUE);
        cout << "========================================" << endl;
        cout << "  当前回合: " << (cur == 1 ? nameA : nameB) << endl;
        cout << "========================================" << endl;
        setColor(COLOR_YELLOW);
        cout << "+--------------------------------------------------+" << endl;
        cout << "| 游戏规则:                                        |" << endl;
        cout << "| 1.踩中地雷 → 扣" << DIRECT_DMG << "血/颗（重叠雷伤害叠加）             |" << endl;
        cout << "| 2.踩雷时9宫格内其他雷上的玩家 → 溅射扣" << SPLASH_DMG << "血/颗      |" << endl;
        cout << "| 3.血瓶 → 回" << HEAL_AMT << "血/个（双方皆可使用）                  |" << endl;
        cout << "| 4.溅射不引爆雷，雷保留在原位                    |" << endl;
        cout << "| 5.埋雷者无免疫，自己踩雷同样受伤                |" << endl;
        cout << "| 6.血量归零出局，存活者获胜                      |" << endl;
        cout << "+--------------------------------------------------+" << endl;
        setColor(COLOR_DEFAULT); cout << endl;

        board.printForGame(posR[1], posC[1], posR[2], posC[2]);

        cout << endl;
        setColor(COLOR_RED);    cout << "  A=" << nameA << "  ";
        setColor(COLOR_BLUE);   cout << "B=" << nameB << "  ";
        setColor(COLOR_MAGENTA);cout << "AB=重叠  ";
        setColor(COLOR_GREEN);  cout << "[+]血瓶";
        setColor(COLOR_DEFAULT);cout << "  [#]未开  [*][@]雷  [!!]重叠雷" << endl << endl;

        // === 输入（WASD移动光标 + P确认，光标在棋盘上实时显示） ===
        int inRow, inCol;
        if (isSingle && cur == 2) {
            // AI 自动选择
            vector< pair<int, int> > cand = board.getUnrevealedCells();
            if (cand.empty()) { over = true; break; }

            setColor(COLOR_CYAN);
            cout << nameB << "(AI) 正在思考..." << endl;
            Sleep(700 + rand() % 1000);

            int idx = rand() % cand.size();
            inRow = cand[idx].first;
            inCol = cand[idx].second;

            setColor(COLOR_YELLOW);
            cout << nameB << "(AI) 移动到: " << (char)('A' + inCol) << (inRow + 1) << endl;
            Sleep(500);
        } else {
            // 手动WASD选择——每步全屏重绘，光标显示在棋盘上
            int cursorR = posR[cur] >= 0 ? posR[cur] : 0;
            int cursorC = posC[cur] >= 0 ? posC[cur] : 0;
            bool confirmed = false;

            while (!confirmed) {
                clearScreen();

                // 重新绘制整个界面（含光标）
                setColor(COLOR_RED);
                cout << nameA << " HP: [";
                int barsa = hp[1] * 2;
                for (int i = 0; i < 20; i++) cout << (i < barsa ? '#' : ' ');
                cout << "] " << hp[1] << "/" << INIT_HP << endl;

                setColor(COLOR_BLUE);
                cout << nameB << " HP: [";
                int barsb = hp[2] * 2;
                for (int i = 0; i < 20; i++) cout << (i < barsb ? '#' : ' ');
                cout << "] " << hp[2] << "/" << INIT_HP;
                if (isSingle) { setColor(COLOR_CYAN); cout << " (AI)"; }
                cout << endl;

                setColor(cur == 1 ? COLOR_RED : COLOR_BLUE);
                cout << "========================================" << endl;
                cout << "  当前回合: " << (cur == 1 ? nameA : nameB)
                     << "  (W/A/S/D移动光标, P-确认)" << endl;
                cout << "========================================" << endl;
                setColor(COLOR_YELLOW);
                cout << "+--------------------------------------------------+" << endl;
                cout << "| 游戏规则:                                        |" << endl;
                cout << "| 1.踩中地雷 → 扣" << DIRECT_DMG << "血/颗（重叠雷伤害叠加）             |" << endl;
                cout << "| 2.踩雷时9宫格内其他雷上的玩家 → 溅射扣" << SPLASH_DMG << "血/颗      |" << endl;
                cout << "| 3.血瓶 → 回" << HEAL_AMT << "血/个（双方皆可使用）                  |" << endl;
                cout << "| 4.溅射不引爆雷，雷保留在原位                    |" << endl;
                cout << "| 5.埋雷者无免疫，自己踩雷同样受伤                |" << endl;
                cout << "| 6.血量归零出局，存活者获胜                      |" << endl;
                cout << "+--------------------------------------------------+" << endl;
                setColor(COLOR_DEFAULT); cout << endl;

                // 用临时位置绘制棋盘：当前玩家位置=光标位置，对方位置不变
                int tmpA_R = (cur == 1) ? cursorR : posR[1];
                int tmpA_C = (cur == 1) ? cursorC : posC[1];
                int tmpB_R = (cur == 2) ? cursorR : posR[2];
                int tmpB_C = (cur == 2) ? cursorC : posC[2];

                board.printForGame(tmpA_R, tmpA_C, tmpB_R, tmpB_C);

                cout << endl;
                setColor(COLOR_RED);    cout << "  A=" << nameA << "  ";
                setColor(COLOR_BLUE);   cout << "B=" << nameB << "  ";
                setColor(COLOR_MAGENTA);cout << "AB=重叠  ";
                setColor(COLOR_GREEN);  cout << "[+]血瓶";
                setColor(COLOR_DEFAULT);cout << "  [#]未开  [*][@]雷  [!!]重叠雷" << endl;

                setColor(COLOR_CYAN);
                cout << "\n光标: " << (char)('A' + cursorC) << (cursorR + 1)
                     << "  (W/A/S/D移动, P-确认选择)" << endl << flush;

                char key = toupper(_getch());

                if (key == 'W' && cursorR > 0) { cursorR--; sfxMove(); }
                else if (key == 'S' && cursorR < BOARD_SIZE - 1) { cursorR++; sfxMove(); }
                else if (key == 'A' && cursorC > 0) { cursorC--; sfxMove(); }
                else if (key == 'D' && cursorC < BOARD_SIZE - 1) { cursorC++; sfxMove(); }
                else if (key == 'P') {
                    if (!firstMove[cur] && board.isRevealed(cursorR, cursorC)) {
                        setColor(COLOR_YELLOW);
                        cout << "\n该格已掀开！按任意键重试..." << flush;
                        _getch();
                    } else {
                        confirmed = true;
                        inRow = cursorR;
                        inCol = cursorC;
                    }
                }
            }
        }

        if (!firstMove[cur] && board.isRevealed(inRow, inCol)) {
            setColor(COLOR_YELLOW); cout << "该格已掀开，请重新选择。"; _getch(); continue;
        }

        // === 移动 ===
        posR[cur] = inRow;
        posC[cur] = inCol;

        if (firstMove[cur]) {
            firstMove[cur] = false;
            board.reveal(inRow, inCol);
            setColor(COLOR_GREEN);
            cout << "初始站位完成！按任意键继续...";
            _getch();
            cur = (cur == 1) ? 2 : 1;
            continue;
        }

        // === 处理踩中效果 ===
        ExplosionResult er = processCell(board, inRow, inCol, cur,
                                          posR[1], posC[1], posR[2], posC[2]);

        // 回复
        hp[cur] += er.heal;
        if (hp[cur] > INIT_HP) hp[cur] = INIT_HP;

        // 直接伤害
        hp[cur] -= er.directDmg;

        // 溅射伤害：检查当前玩家是否在溅射雷的9宫格内
        // 溅射已经在processCell中处理了雷的清除，伤害需要应用到对应玩家
        if (er.splashDmgA > 0) hp[1] -= er.splashDmgA;
        if (er.splashDmgB > 0) hp[2] -= er.splashDmgB;

        // 另外，如果引爆的雷在对方当前位置的9宫格内，对方也受溅射
        // 已经在processCell处理

        // 输出日志
        for (size_t i = 0; i < er.log.size(); i++) {
            setColor(COLOR_YELLOW);
            cout << er.log[i] << endl;
        }

        // 显示当前位置
        setColor(COLOR_CYAN);
        cout << (cur == 1 ? nameA : nameB) << " 现在站在 " << (char)('A' + inCol) << (inRow + 1) << endl;

        // 检查死亡
        if (hp[1] <= 0 && hp[2] <= 0) {
            over = true;
            // 双方都死
        } else if (hp[1] <= 0) {
            over = true;
        } else if (hp[2] <= 0) {
            over = true;
        }

        cout << "\n按任意键继续...";
        _getch();

        if (over) break;
        if (board.allSafeRevealed()) { over = true; break; }
        cur = (cur == 1) ? 2 : 1;
    }

    // 终局展示
    clearScreen();
    board.revealAll();

    setColor(COLOR_WHITE);
    cout << "========================================" << endl;
    if (hp[1] <= 0 && hp[2] <= 0) {
        setColor(COLOR_YELLOW); cout << "        双方同归于尽！平局！" << endl;
        sfxDraw();
    } else if (hp[1] <= 0) {
        setColor(COLOR_RED); cout << "  " << nameA << " 阵亡！" << endl;
        setColor(COLOR_GREEN); cout << "  胜利者: " << nameB << "！" << endl;
        sfxWin();
    } else if (hp[2] <= 0) {
        setColor(COLOR_RED); cout << "  " << nameB << " 阵亡！" << endl;
        setColor(COLOR_GREEN); cout << "  胜利者: " << nameA << "！" << endl;
        sfxWin();
    } else {
        setColor(COLOR_GREEN); cout << "        平局！双方均存活！" << endl;
        sfxDraw();
    }
    cout << "  " << nameA << " 剩余血量: " << (hp[1] > 0 ? hp[1] : 0) << endl;
    cout << "  " << nameB << " 剩余血量: " << (hp[2] > 0 ? hp[2] : 0) << endl;
    cout << "========================================" << endl;
    setColor(COLOR_RED);    cout << "* = A雷  ";
    setColor(COLOR_BLUE);   cout << "@ = B雷  ";
    setColor(COLOR_YELLOW); cout << "!! = 重叠雷  ";
    setColor(COLOR_GREEN);  cout << "+ = 血瓶";
    setColor(COLOR_DEFAULT);cout << endl << endl;

    board.printFinal();
    cout << endl;
}

// ===================== Main =====================
int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    srand((unsigned int)time(NULL));

    clearScreen();

    setColor(COLOR_YELLOW);
    cout << "+========================================+" << endl;
    cout << "|                                        |" << endl;
    cout << "|        双 人 扫 雷 对 决               |" << endl;
    cout << "|                                        |" << endl;
    cout << "|  中国阵营 *  VS  他国阵营 @            |" << endl;
    cout << "|                                        |" << endl;
    cout << "+========================================+" << endl;
    cout << endl;

    // 模式选择
    setColor(COLOR_CYAN);
    cout << "请选择游戏模式:" << endl;
    cout << "  1 - 单人模式 ( vs AI )" << endl;
    cout << "  2 - 双人模式 ( vs 好友 )" << endl;
    setColor(COLOR_YELLOW);
    cout << "请输入 (1 或 2): ";
    char mode = toupper(_getch()); cout << mode << endl;
    while (mode != '1' && mode != '2') {
        cout << "无效选择，请输入 1 或 2: ";
        mode = toupper(_getch()); cout << mode << endl;
    }
    bool isSingle = (mode == '1');

    cout << endl;
    setColor(COLOR_CYAN);
    cout << (isSingle ? "=== 单人模式 ===" : "=== 双人模式 ===") << endl;
    cout << endl;

    // 规则
    setColor(COLOR_CYAN);
    cout << "游戏规则:" << endl;
    cout << "  1. 双方秘密布置 " << MINES_EACH << " 个地雷 + " << HPOT_EACH << " 个血瓶（可重叠）" << endl;
    cout << "  2. 每人初始血量: " << INIT_HP << " 点" << endl;
    cout << "  3. 踩中地雷 → 扣 " << DIRECT_DMG << " 血/颗（重叠雷伤害翻倍）" << endl;
    cout << "  4. 周围9宫格内对方雷被引爆 → 溅射伤害 " << SPLASH_DMG << " 血/颗" << endl;
    cout << "  5. 踩中血瓶 → 回复 " << HEAL_AMT << " 血/个（双方都可使用）" << endl;
    cout << "  6. 血量归零则出局，存活者获胜" << endl;
    if (isSingle) cout << "  7. 单人模式下，玩家B由AI操控" << endl;
    cout << endl;

    // 名称
    setColor(COLOR_YELLOW); cout << "请设置玩家名称:" << endl;
    setColor(COLOR_RED); cout << "玩家A(中国阵营 *): ";
    string nameA, nameB;
    getline(cin, nameA);
    if (nameA.empty()) nameA = "玩家A";

    if (!isSingle) {
        setColor(COLOR_BLUE); cout << "玩家B(他国阵营 @): ";
        getline(cin, nameB);
        if (nameB.empty()) nameB = "玩家B";
    } else {
        nameB = "AI对手";
        setColor(COLOR_BLUE); cout << "玩家B(他国阵营 @): " << nameB << " (AI)" << endl;
    }
    cout << endl;

    Board board;

    // A 布雷
    placementPhase(board, 1, nameA + "(中国阵营 *)", MINES_EACH, HPOT_EACH);

    // B 布雷
    if (isSingle) {
        aiPlaceItems(board, 2, MINES_EACH, HPOT_EACH);
        clearScreen();
        setColor(COLOR_GREEN);
        cout << nameB << "(AI) 已自动完成布雷。" << endl;
        cout << "按任意键开始游戏...";
        _getch();
    } else {
        placementPhase(board, 2, nameB + "(他国阵营 @)", MINES_EACH, HPOT_EACH);
    }

    // 开场音效
    sfxGameStart();

    // 游戏
    gamePlayPhase(board, nameA, nameB, isSingle);

    setColor(COLOR_WHITE);
    cout << "按任意键退出...";
    _getch();
    return 0;
}