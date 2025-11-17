// Escape The Maze - Boss Battle Edition
//fISHER corridors ko pack nai krta kisi cheez ko band nai krta
//Flood apki bush spawn krta hy apko kisi bush ky upr spawn nai krta
/*
Mashood Khan (CT-24021)
Ibtisam Khan (CT-24023)
Syed Wahaaj Ali (CT-24035)
*/
//g++ escape_game.cpp -o escape_game.exe $(pkg-config --cflags --libs raylib) -lwinmm -lgdi32 -lopengl32

// Compile: g++ main.cpp -o main.exe $(pkg-config --cflags --libs raylib) -lwinmm -lgdi32 -lopengl32

#include "raylib.h"
#include <cmath>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <random>
#include <queue>

using namespace std;

// Game Constants
const int CELL = 48;
const int MIN_MARGIN = 80;
const int BOSS_LEVEL = 5;
const int BOSS_LEVEL_KEY_COUNT = 10;
const float BOSS_ARRIVAL_TIME = 2.0f;
const float LASER_COOLDOWN = 5.0f;
const float BOSS_RADIUS = 110.0f;
const float PLAYER_RADIUS = 15.0f;
const float ENEMY_RADIUS = 15.0f;
const float PLAYER_SPEED = 220.0f;

// Utility Functions
float Clamp(float value, float min, float max) {
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

float Distance(Vector2 a, Vector2 b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return sqrtf(dx*dx + dy*dy);
}

Vector2 Vector2Normalize(Vector2 v) {
	float len = sqrtf(v.x * v.x + v.y * v.y);
	if (len == 0.0f) return {0.0f, 0.0f};
	return {v.x / len, v.y / len};
}

float Vector2DotProduct(Vector2 v1, Vector2 v2) {
	return v1.x * v2.x + v1.y * v2.y;
}

float Vector2DistanceToLine(Vector2 p, Vector2 a, Vector2 b) {
	Vector2 ap = {p.x - a.x, p.y - a.y};
	Vector2 ab = {b.x - a.x, b.y - a.y};
	float magnitudeSq = Vector2DotProduct(ab, ab);
	if (magnitudeSq == 0.0f) return Distance(p, a);
	float u = Vector2DotProduct(ap, ab) / magnitudeSq;
	u = Clamp(u, 0.0f, 1.0f);
	Vector2 closest = {a.x + u * ab.x, a.y + u * ab.y};
	return Distance(p, closest);
}

// Game Structures
struct Bush {
	Rectangle area;
};

struct Key {
	Vector2 pos;
	bool collected = false;
};

struct Chest {
	Vector2 pos;
	bool opened = false;
	int effect = 0;
};

enum EnemyType { TYPE_STANDARD, TYPE_BOSS, TYPE_MINION };

struct Enemy {
	Vector2 pos;
	float speed;
	float radius = ENEMY_RADIUS;
	float visionRange;
	float visionGrowRate;
	EnemyType type = TYPE_STANDARD;
	Vector2 moveDirection = {0.0f, 0.0f};
	vector<Vector2> patrolPoints;
	int currentPatrol = 0;
	float patrolWait = 0.0f;
	bool released = false;
	int gx = -1, gy = -1;
	Vector2 currentTarget;
	bool hasTarget = false;
};

// Helper Functions
Vector2 RandomPoint(int screenWidth, int screenHeight) {
	return {
		(float)GetRandomValue(MIN_MARGIN, screenWidth - MIN_MARGIN),
		(float)GetRandomValue(MIN_MARGIN, screenHeight - MIN_MARGIN)
	};
}

bool CircleCollidesRect(const Vector2 &center, float radius, const Rectangle &rect) {
	return CheckCollisionCircleRec(center, radius, rect);
}

bool CircleCollidesAnyBush(const Vector2 &center, float radius, const vector<Bush> &bushes) {
	for (const auto &b : bushes) {
		if (CircleCollidesRect(center, radius, b.area)) return true;
	}
	return false;
}

float GetChaseIntensity(const vector<Enemy>& enemies, Vector2 player) {
	float minDist = 1e9f;
	for (auto &e : enemies) {
		if (!e.released) continue;
		float dist = Distance(e.pos, player);
		if (dist < minDist) minDist = dist;
	}
	float intensity = 0.0f;
	if (minDist < 500.0f) intensity = 1.0f - (minDist / 500.0f);
	return Clamp(intensity, 0.0f, 1.0f);
}

// Maze Generation
struct GridState {
	int gw, gh;
	vector<vector<int>> grid;

	GridState(int gh_, int gw_) : gw(gw_), gh(gh_) {
		grid.assign(gh, vector<int>(gw, 1));
	}
};

void WorldToGrid(const Vector2 &w, int &gx, int &gy, int cellSize) {
	gx = (int)(w.x / cellSize);
	gy = (int)(w.y / cellSize);
}

Vector2 GridToWorldCenter(int gx, int gy, int cellSize) {
	return {
		gx * (float)cellSize + cellSize * 0.5f,
		gy * (float)cellSize + cellSize * 0.5f
	};
}
//dfs
//backtracking
void MazeCarve(GridState &G, int sy, int sx, int openness, default_random_engine &rng) {
	static int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
	vector<int> order = {0,1,2,3};
	shuffle(order.begin(), order.end(), rng); //fisher maze banany ky lie use hua

	for (int d : order) {
		int ny = sy + dirs[d][0]*2;
		int nx = sx + dirs[d][1]*2;
		if (ny >= 1 && ny < G.gh-1 && nx >= 1 && nx < G.gw-1 && G.grid[ny][nx] == 1) {
			if ((rand() % 100) < openness) continue;
			G.grid[sy + dirs[d][0]][sx + dirs[d][1]] = 0;
			G.grid[ny][nx] = 0;
			MazeCarve(G, ny, nx, openness, rng);
		}
	}
}

void CreateOpenAreas(GridState &G, int count, int minSize, int maxSize, default_random_engine &rng) {
	uniform_int_distribution<int> distX(2, G.gw - maxSize - 2);
	uniform_int_distribution<int> distY(2, G.gh - maxSize - 2);
	uniform_int_distribution<int> distSize(minSize, maxSize);

	for (int i = 0; i < count; i++) {
		int rx = distX(rng), ry = distY(rng);
		int rw = distSize(rng), rh = distSize(rng);
		if (rx + rw >= G.gw - 1 || ry + rh >= G.gh - 1) continue;

		for (int y = ry; y < ry + rh && y < G.gh - 1; y++) {
			for (int x = rx; x < rx + rw && x < G.gw - 1; x++) {
				G.grid[y][x] = 0;
			}
		}
	}
}

void CreateCorridors(GridState &G, int level) {
	int hCorridors = 2 + level / 2;
	for (int i = 0; i < hCorridors; i++) {
		int y = 2 + (rand() % (G.gh - 4));
		for (int x = 1; x < G.gw - 1; x++) {
			if (rand() % 100 < 70) G.grid[y][x] = 0;
		}
	}

	int vCorridors = 2 + level / 2;
	for (int i = 0; i < vCorridors; i++) {
		int x = 2 + (rand() % (G.gw - 4));
		for (int y = 1; y < G.gh - 1; y++) {
			if (rand() % 100 < 70) G.grid[y][x] = 0;
		}
	}
}
//bfs
vector<Bush> GenerateBushesFromMaze(int level, int sw, int sh, const Vector2 &playerStart,
                                    const Rectangle &door, const vector<Key> &existingKeys,
                                    const Chest &existingChest, const vector<Enemy> &existingEnemies) {
	int gw = max(9, (sw - MIN_MARGIN*2) / CELL);
	int gh = max(9, (sh - MIN_MARGIN*2) / CELL);
	if (gw % 2 == 0) gw--;
	if (gh % 2 == 0) gh--;

	int openness = 50 - (level - 1) * 10;
	if (level == BOSS_LEVEL) openness = 10;
	openness = Clamp(openness, 10, 50);

	default_random_engine rng((unsigned)time(NULL) + level*1013 + rand()%1000);
	vector<Bush> bushes;

	auto overlapsForbidden = [&](float bx, float by, float size) -> bool {
		Rectangle r = {bx, by, size, size};
		if (CheckCollisionCircleRec(playerStart, 40.0f, r)) return true;
		if (CheckCollisionRecs(r, door)) return true;

		for (auto &k : existingKeys) {
			if (CheckCollisionCircleRec(k.pos, 30.0f, r)) return true;
		}
		if (CheckCollisionCircleRec(existingChest.pos, 30.0f, r)) return true;

		for (auto &e : existingEnemies) {
			if (e.type != TYPE_MINION && CheckCollisionCircleRec(e.pos, e.radius + 10.0f, r)) {
				return true;
			}
		}

		if (bx < MIN_MARGIN || by < MIN_MARGIN ||
		        bx + size > sw - MIN_MARGIN || by + size > sh - MIN_MARGIN) {
			return true;
		}
		return false;
	};

	int attempts = 0;
	while (attempts++ < 30) {
		GridState G(gh, gw);
		for (int y=0; y<gh; y++) {
			for (int x=0; x<gw; x++) {
				G.grid[y][x] = 1;
			}
		}

		int sy = 1 + 2*(rand() % max(1, (gh-1)/2));
		int sx = 1 + 2*(rand() % max(1, (gw-1)/2));
		G.grid[sy][sx] = 0;
		MazeCarve(G, sy, sx, openness, rng);

		int openAreaCount = max(1, 6 - level);
		if (level == BOSS_LEVEL) openAreaCount = 3;
		CreateOpenAreas(G, openAreaCount, 2, 4, rng);
		CreateCorridors(G, level);

		int removePct = 40 - (level - 1) * 6;
		if (level == BOSS_LEVEL) removePct = 20;
		removePct = Clamp(removePct, 15, 40);

		for (int y=1; y<gh-1; y++) {
			for (int x=1; x<gw-1; x++) {
				if (G.grid[y][x] == 1 && (rand()%100) < removePct) {
					G.grid[y][x] = 0;
				}
			}
		}

		int borderOpenings = 6 + (5 - level) * 2;
		for (int i = 0; i < borderOpenings / 2; i++) {
			int y = 2 + (rand() % (G.gh - 4));
			G.grid[y][1] = 0;
			G.grid[y][0] = 0;
		}
		for (int i = 0; i < borderOpenings / 2; i++) {
			int y = 2 + (rand() % (G.gh - 4));
			G.grid[y][gw-2] = 0;
			G.grid[y][gw-1] = 0;
		}
		for (int i = 0; i < borderOpenings / 2; i++) {
			int x = 2 + (rand() % (gw - 4));
			G.grid[1][x] = 0;
			G.grid[0][x] = 0;
		}
		for (int i = 0; i < borderOpenings / 2; i++) {
			int x = 2 + (rand() % (gw - 4));
			G.grid[gh-2][x] = 0;
			G.grid[gh-1][x] = 0;
		}

		int pgx = Clamp((int)(playerStart.x / CELL), 0, gw-1);
		int pgy = Clamp((int)(playerStart.y / CELL), 0, gh-1);
		int d_gx = Clamp((int)(door.x / CELL), 0, gw-1);
		int d_gy = Clamp((int)(door.y / CELL), 0, gh-1);

		vector<vector<char>> vis(gh, vector<char>(gw,0));
		queue<pair<int,int>> q;  //bfs
		if (G.grid[pgy][pgx] == 0) {
			q.push({pgy,pgx});
			vis[pgy][pgx]=1;
		}

		while (!q.empty()) {
			auto [cy,cx] = q.front();
			q.pop();
			static int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
			for (auto &d: dirs) {
				int ny = cy + d[0];
				int nx = cx + d[1];
				if (ny>=0 && ny<gh && nx>=0 && nx<gw && !vis[ny][nx] && G.grid[ny][nx]==0) {
					vis[ny][nx]=1;
					q.push({ny,nx});
				}
			}
		}

		if (!vis[d_gy][d_gx]) continue;

		bushes.clear();
		for (int y=0; y<gh; y++) {
			for (int x=0; x<gw; x++) {
				if (G.grid[y][x] == 1) {
					float bx = x * CELL;
					float by = y * CELL;
					float pad = 2.0f;
					Rectangle r = {bx + pad, by + pad, CELL - pad*2, CELL - pad*2};

					if (overlapsForbidden(r.x, r.y, r.width)) continue;
					Bush b;
					b.area = r;
					bushes.push_back(b);
				}
			}
		}

		for (int i = 0; i < (int)bushes.size(); i++) {
			for (int j = i+1; j < (int)bushes.size();) {
				if (CheckCollisionRecs(bushes[i].area, bushes[j].area)) {
					float areaI = bushes[i].area.width * bushes[i].area.height;
					float areaJ = bushes[j].area.width * bushes[j].area.height;
					if (areaI >= areaJ) {
						bushes.erase(bushes.begin()+j);
					} else {
						bushes.erase(bushes.begin()+i);
						i=-1;
						break;
					}
				} else {
					j++;
				}
			}
		}
		return bushes;
	}
	return vector<Bush>();
}
//saves from gliches bush ky andr kisi cheez ko spawn nai hony dyta
Vector2 GetRandomFreeCell(int sw, int sh, const vector<Bush> &bushes) {
	int gw = max(5, (sw - MIN_MARGIN*2) / CELL);
	int gh = max(5, (sh - MIN_MARGIN*2) / CELL);
	if (gw % 2 == 0) gw--;
	if (gh % 2 == 0) gh--;

	vector<vector<int>> occ(gh, vector<int>(gw, 0));
	for (auto &b : bushes) {
		int x1 = (int)floor(b.area.x / CELL);
		int y1 = (int)floor(b.area.y / CELL);
		int x2 = (int)floor((b.area.x + b.area.width) / CELL);
		int y2 = (int)floor((b.area.y + b.area.height) / CELL);
		x1 = Clamp(x1, 0, gw-1);
		y1 = Clamp(y1, 0, gh-1);
		x2 = Clamp(x2, 0, gw-1);
		y2 = Clamp(y2, 0, gh-1);

		for (int yy=y1; yy<=y2; yy++) {
			for (int xx=x1; xx<=x2; xx++) {
				occ[yy][xx] = 1;
			}
		}
	}

	vector<pair<int,int>> freeCells;
	for (int y = 1; y < gh-1; y++) {
		for (int x = 1; x < gw-1; x++) {
			if (occ[y][x] == 0) freeCells.push_back({y, x});
		}
	}

	if (freeCells.empty()) return {(float)sw/2, (float)sh/2};
	int idx = rand() % freeCells.size();
	return GridToWorldCenter(freeCells[idx].second, freeCells[idx].first, CELL);
}
//bfs enemy intelligent movement
Vector2 EnemyNextTargetOnGrid(const Enemy &e, const Vector2 &targetPos, int sw, int sh, const vector<Bush> &bushes) {
	int gw = max(5, (sw - MIN_MARGIN*2) / CELL);
	int gh = max(5, (sh - MIN_MARGIN*2) / CELL);
	if (gw % 2 == 0) gw--;
	if (gh % 2 == 0) gh--;

	vector<vector<int>> occ(gh, vector<int>(gw, 0));
	for (auto &b : bushes) {
		int x1 = (int)floor(b.area.x / CELL);
		int y1 = (int)floor(b.area.y / CELL);
		int x2 = (int)floor((b.area.x + b.area.width) / CELL);
		int y2 = (int)floor((b.area.y + b.area.height) / CELL);
		x1 = Clamp(x1, 0, gw-1);
		y1 = Clamp(y1, 0, gh-1);
		x2 = Clamp(x2, 0, gw-1);
		y2 = Clamp(y2, 0, gh-1);

		for (int yy=y1; yy<=y2; yy++) {
			for (int xx=x1; xx<=x2; xx++) {
				occ[yy][xx] = 1;
			}
		}
	}

	int sx = Clamp((int)(e.pos.x / CELL), 0, gw-1);
	int sy = Clamp((int)(e.pos.y / CELL), 0, gh-1);
	int tx = Clamp((int)(targetPos.x / CELL), 0, gw-1);
	int ty = Clamp((int)(targetPos.y / CELL), 0, gh-1);

	if (sx == tx && sy == ty) return targetPos;

	vector<vector<char>> vis(gh, vector<char>(gw,0));
	vector<vector<pair<int,int>>> parent(gh, vector<pair<int,int>>(gw, {-1,-1}));
	queue<pair<int,int>> q;

	if (occ[sy][sx]==0) {
		q.push({sy,sx});
		vis[sy][sx]=1;
		parent[sy][sx] = {sy,sx};
	} else {
		return targetPos;
	}

	static int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
	while (!q.empty()) {
		auto [cy,cx] = q.front();
		q.pop();
		if (cy==ty && cx==tx) break;

		for (auto &d : dirs) {
			int ny = cy + d[0];
			int nx = cx + d[1];
			if (ny>=0 && ny<gh && nx>=0 && nx<gw && !vis[ny][nx] && occ[ny][nx]==0) {
				vis[ny][nx]=1;
				parent[ny][nx] = {cy,cx};
				q.push({ny,nx});
			}
		}
	}

	if (!vis[ty][tx]) return targetPos;

	pair<int,int> cur = {ty,tx};
	while (!(parent[cur.first][cur.second].first == sy && parent[cur.first][cur.second].second == sx)) {
		cur = parent[cur.first][cur.second];
		if (cur.first == -1) return targetPos;
	}
	return GridToWorldCenter(cur.second, cur.first, CELL);
}

vector<Key> CreateKeys(int count, int sw, int sh) {
	vector<Key> keys;
	for (int i = 0; i < count; i++) {
		keys.push_back({RandomPoint(sw, sh), false});
	}
	return keys;
}

vector<Enemy> CreateEnemies(int count, int sw, int sh, const vector<Bush> &bushes) {
	vector<Enemy> enemies;
	for (int i = 0; i < count; i++) {
		Enemy e;
		e.type = TYPE_STANDARD;
		e.pos = GetRandomFreeCell(sw, sh, bushes);
		e.speed = 130.0f + GetRandomValue(0, 30);
		e.visionRange = 200.0f;
		e.visionGrowRate = 5.0f;
		for (int j = 0; j < 4; j++) {
			e.patrolPoints.push_back(GetRandomFreeCell(sw, sh, bushes));
		}
		e.currentTarget = e.pos;
		e.hasTarget = false;
		enemies.push_back(e);
	}
	return enemies;
}
//jb ham out hojain mtlb 0 live hojai tw sb cheezen reset hojati hy
void ResetLevel(Vector2 &player, vector<Enemy> &enemies, vector<Key> &keys, vector<Bush> &bushes,
                Chest &chest, int &collectedKeys, int &totalKeys, float &enemyCountdown,
                float &levelTimer, bool &levelActive, bool &bossBattleActive, float &minionSpawnTimer,
                float normalSpeed, int level, int sw, int sh, float &laserCountdown, bool &laserActive) {

	player = {(float)sw / 2, (float)sh / 2};
	enemies.clear();
	Rectangle door = {(float)sw - 120, (float)sh / 2 - 40, 60, 80};
	bossBattleActive = false;
	minionSpawnTimer = 0.0f;
	laserActive = false;

	if (level == BOSS_LEVEL) {
		totalKeys = BOSS_LEVEL_KEY_COUNT;
		keys = CreateKeys(totalKeys, sw, sh);
		chest = {RandomPoint(sw, sh), false, 0};
		bushes = GenerateBushesFromMaze(level, sw, sh, player, door, keys, chest, {});

		Enemy boss;
		boss.type = TYPE_BOSS;
		boss.radius = BOSS_RADIUS;
		boss.pos = {(float)sw / 2, -boss.radius * 3};
		boss.speed = normalSpeed * 0.5f;
		boss.visionRange = 400.0f;
		boss.visionGrowRate = 0.0f;
		boss.released = false;
		enemies.push_back(boss);

		bossBattleActive = true;
		levelTimer = 90.0f;
		enemyCountdown = BOSS_ARRIVAL_TIME;
		minionSpawnTimer = 3.0f;
		laserCountdown = LASER_COOLDOWN;
	} else {
		totalKeys = 2 + level;
		keys = CreateKeys(totalKeys, sw, sh);
		chest = {RandomPoint(sw, sh), false, 0};
		bushes = GenerateBushesFromMaze(level, sw, sh, player, door, keys, chest, {});
		enemies = CreateEnemies(level, sw, sh, bushes);
		enemyCountdown = 5.0f;
		levelTimer = 60.0f;
		laserCountdown = 0.0f;
	}

	collectedKeys = 0;
	levelActive = true;
}

// Drawing Functions
void DrawPlayer(Vector2 pos, float radius, bool isHidden, float time) {
	Color bodyColor = isHidden ? Fade(BLUE, 0.4f) : BLUE;
	DrawCircleV(pos, radius, bodyColor);
	DrawCircleLines(pos.x, pos.y, radius, Fade(DARKBLUE, 0.8f));

	if (!isHidden) {
		float eyeOffset = radius * 0.3f;
		DrawCircle(pos.x - eyeOffset, pos.y - 2, 2, WHITE);
		DrawCircle(pos.x + eyeOffset, pos.y - 2, 2, WHITE);
		DrawCircle(pos.x - eyeOffset, pos.y - 2, 1, BLACK);
		DrawCircle(pos.x + eyeOffset, pos.y - 2, 1, BLACK);
	}

	float pulse = 0.3f + 0.2f * sinf(time * 3.0f);
	DrawCircleLines(pos.x, pos.y, radius + 3, Fade(SKYBLUE, pulse));
}

void DrawEnemy(const Enemy &e, float time, float enemyCountdown) {
	if (e.type == TYPE_BOSS) {
		float radius = e.radius;

		if (!e.released) {
			float progress = Clamp(1.0f - enemyCountdown / BOSS_ARRIVAL_TIME, 0.0f, 1.0f);
			float maxGlowRadius = 250.0f;
			float currentRadius = maxGlowRadius * (1.0f - progress);
			DrawCircleV(e.pos, currentRadius, Fade(ORANGE, 1.0f - progress));
			DrawCircleV(e.pos, currentRadius * 0.8f, Fade(RED, 1.0f - progress));
			DrawText("!! BOSS INCOMING !!", e.pos.x - 100, e.pos.y - 100, 20, YELLOW);
		} else {
			DrawCircleV(e.pos, radius, DARKPURPLE);
			DrawCircleLines(e.pos.x, e.pos.y, radius, MAGENTA);

			float pulse = 0.6f + 0.4f * sinf(time * 5.0f);
			DrawCircleLines(e.pos.x, e.pos.y, radius + 4, Fade(MAGENTA, pulse));

			float eyeOffset = radius * 0.35f;
			DrawCircle(e.pos.x - eyeOffset, e.pos.y - 10, 5, YELLOW);
			DrawCircle(e.pos.x + eyeOffset, e.pos.y - 10, 5, YELLOW);
			DrawCircle(e.pos.x - eyeOffset, e.pos.y - 10, 2, BLACK);
			DrawCircle(e.pos.x + eyeOffset, e.pos.y - 10, 2, BLACK);
			DrawText("BOSS", e.pos.x - 20, e.pos.y - 55, 16, MAGENTA);
		}
	} else if (e.type == TYPE_MINION) {
		float radius = e.radius;
		Rectangle minionRect = {e.pos.x - radius, e.pos.y - radius, radius*2, radius*2};
		DrawRectangleRec(minionRect, RED);
		DrawRectangleLines(e.pos.x - radius, e.pos.y - radius, radius*2, radius*2, MAROON);

		float pulse = 0.4f + 0.3f * sinf(time * 4.0f);
		Rectangle glowRect = {e.pos.x - radius - 2, e.pos.y - radius - 2, radius*2 + 4, radius*2 + 4};
		DrawRectangleLinesEx(glowRect, 1, Fade(ORANGE, pulse));
	} else {
		float radius = e.radius;
		DrawCircleV(e.pos, radius, RED);
		DrawCircleLines(e.pos.x, e.pos.y, radius, MAROON);

		float eyeOffset = radius * 0.35f;
		DrawCircle(e.pos.x - eyeOffset, e.pos.y - 3, 3, YELLOW);
		DrawCircle(e.pos.x + eyeOffset, e.pos.y - 3, 3, YELLOW);
		DrawCircle(e.pos.x - eyeOffset, e.pos.y - 3, 1, BLACK);
		DrawCircle(e.pos.x + eyeOffset, e.pos.y - 3, 1, BLACK);

		DrawLineEx({e.pos.x - eyeOffset - 3, e.pos.y - 6}, {e.pos.x - eyeOffset + 3, e.pos.y - 8}, 1.5f, BLACK);
		DrawLineEx({e.pos.x + eyeOffset - 3, e.pos.y - 8}, {e.pos.x + eyeOffset + 3, e.pos.y - 6}, 1.5f, BLACK);

		float pulse = 0.4f + 0.3f * sinf(time * 4.0f);
		DrawCircleLines(e.pos.x, e.pos.y, radius + 2, Fade(ORANGE, pulse));
	}
}

void DrawKey(Vector2 pos, float time) {
	float sparkle = 0.7f + 0.3f * sinf(time * 5.0f);
	float size = 14.0f;

	DrawCircleV(pos, size + 4, Fade(YELLOW, 0.2f * sparkle));
	DrawCircleV(pos, size, Fade(GOLD, sparkle));
	DrawCircleLines(pos.x, pos.y, size, ORANGE);
	DrawCircle(pos.x, pos.y, 5, ORANGE);

	for (int i = 0; i < 4; i++) {
		float angle = time * 2.0f + i * PI / 2;
		float sparkX = pos.x + cosf(angle) * (size + 6);
		float sparkY = pos.y + sinf(angle) * (size + 6);
		DrawCircle(sparkX, sparkY, 2, Fade(WHITE, sparkle));
	}
}

void DrawExitDoor(Rectangle door, bool unlocked, float time) {
	Color frameColor = unlocked ? DARKGREEN : DARKGRAY;
	Rectangle outerFrame = {door.x - 4, door.y - 4, door.width + 8, door.height + 8};
	DrawRectangleLinesEx(outerFrame, 4, frameColor);

	Color doorColor = unlocked ? GREEN : GRAY;
	DrawRectangleRec(door, doorColor);

	float panelPad = 8;
	Rectangle topPanel = {door.x + panelPad, door.y + panelPad, door.width - panelPad*2, door.height/2 - panelPad*1.5f};
	Rectangle bottomPanel = {door.x + panelPad, door.y + door.height/2 + panelPad/2, door.width - panelPad*2, door.height/2 - panelPad*1.5f};
	DrawRectangleLinesEx(topPanel, 2, frameColor);
	DrawRectangleLinesEx(bottomPanel, 2, frameColor);

	if (unlocked) {
		float glow = 0.5f + 0.5f * sinf(time * 3.0f);
		DrawRectangleLinesEx(door, 3, Fade(LIME, glow));
		DrawText("EXIT", door.x + 8, door.y + 28, 24, Fade(WHITE, glow));
	} else {
		int lockX = door.x + door.width/2 - 8;
		int lockY = door.y + door.height/2 - 8;
		DrawRectangle(lockX, lockY + 8, 16, 12, DARKGRAY);
		DrawCircleLines(lockX + 8, lockY + 4, 6, DARKGRAY);
		DrawText("LOCKED", door.x + 4, door.y + 28, 16, DARKGRAY);
	}
}

void DrawStyledHUD(int level, int collectedKeys, int totalKeys, int lives, float timeLeft,
                   bool levelActive, float enemyCountdown, int sw, bool blackout) {
	Color bgColor = blackout ? Fade(BLACK, 0.7f) : Fade(BLACK, 0.5f);
	Color textColor = WHITE;
	Color accentColor = YELLOW;

	Rectangle hudBox = {10, 10, 280, 180};
	DrawRectangle(hudBox.x, hudBox.y, hudBox.width, hudBox.height, bgColor);
	DrawRectangleLinesEx(hudBox, 2, accentColor);

	int yPos = 25;
	int xPad = 25;

	Color levelColor = (level == BOSS_LEVEL) ? MAGENTA : accentColor;
	const char* levelText = (level == BOSS_LEVEL) ? " (BOSS)" : "";
	DrawText(TextFormat("LEVEL: %d%s", level, levelText), xPad, yPos, 24, levelColor);
	yPos += 35;

	Color keyColor = (collectedKeys == totalKeys) ? LIME : textColor;
	DrawCircle(xPad + 8, yPos + 8, 6, GOLD);
	DrawText(TextFormat("KEYS: %d / %d", collectedKeys, totalKeys), xPad + 25, yPos, 20, keyColor);
	yPos += 35;

	DrawText("LIVES: ", xPad, yPos, 20, textColor);
	for (int i = 0; i < 3; i++) {
		Color heartColor = (i < lives) ? RED : Fade(DARKGRAY, 0.5f);
		float hx = xPad + 80 + i * 25;
		DrawCircle(hx - 3, yPos + 8, 5, heartColor);
		DrawCircle(hx + 3, yPos + 8, 5, heartColor);
		DrawTriangle({hx - 8.0f, (float)(yPos + 8)}, {hx + 8.0f, (float)(yPos + 8)}, {hx, (float)(yPos + 16)}, heartColor);
	}
	yPos += 35;

	if (levelActive) {
		Color timerColor = timeLeft < 10.0f ? RED : textColor;
		DrawText(TextFormat("TIME: %.1f", timeLeft), xPad, yPos, 20, timerColor);

		float barWidth = 200.0f;
		float maxTime = (level == BOSS_LEVEL) ? 90.0f : 60.0f;
		float fillWidth = (timeLeft / maxTime) * barWidth;
		DrawRectangle(xPad, yPos + 25, barWidth, 8, Fade(DARKGRAY, 0.5f));
		DrawRectangle(xPad, yPos + 25, fillWidth, 8, timerColor);
	}
	yPos += 40;

	if (level == BOSS_LEVEL && enemyCountdown > 0.0f) {
		DrawText("BOSS INCOMING!", xPad, yPos, 18, MAGENTA);
	} else if (level != BOSS_LEVEL && enemyCountdown > 0.0f) {
		DrawText(TextFormat("ENEMIES IN: %.1f", enemyCountdown), xPad, yPos, 18, ORANGE);
	}
}

void DrawMenuScreen(int sw, int sh, float time) {
	DrawRectangleGradientV(0, 0, sw, sh, Fade(DARKBLUE, 0.9f), Fade(DARKPURPLE, 0.9f));

	for (int i = 0; i < 20; i++) {
		float x = (sw / 2) + sinf(time * 0.5f + i) * (300.0f + i * 20);
		float y = (sh / 3) + cosf(time * 0.3f + i * 0.5f) * 100.0f;
		float size = 3.0f + sinf(time + i) * 2.0f;
		DrawCircle(x, y, size, Fade(SKYBLUE, 0.3f));
	}

	const char* title = "ESCAPE THE MAZE";
	int fontSize = 90;
	int boldOffset = 2;

	int titleWidth = MeasureText(title, fontSize);
	int titleX = (sw - titleWidth) / 2;
	int titleY = sh / 5;

	for (int ox = -boldOffset; ox <= boldOffset; ox++) {
		for (int oy = -boldOffset; oy <= boldOffset; oy++) {
			DrawText(title, titleX + ox, titleY + oy, fontSize, Fade(BLACK, 0.6f));
		}
	}

	DrawText(title, titleX, titleY, fontSize, GOLD);
	DrawText("Navigate mazes, collect keys, escape enemies!",
	         (sw / 2) - 250, titleY + 100, 20, WHITE);


	float pulse = 0.7f + 0.3f * sinf(time * 4.0f);
	Rectangle btn = { (float)sw/2 - 180, (float)(titleY + 160), 360, 60 };

	DrawRectangleRec(btn, Fade(GREEN, pulse * 0.8f));
	DrawRectangleLinesEx(btn, 3, Fade(LIME, pulse));
	DrawText("PRESS ENTER TO START", sw / 2 - 160, titleY + 175, 24, WHITE);

	int ctrlY = titleY + 260;

	DrawText("CONTROLS:", sw / 2 - 60, ctrlY, 18, SKYBLUE);
	DrawText("WASD / Arrow Keys - Move", sw / 2 - 130, ctrlY + 30, 16, WHITE);

	DrawText("Project By:", sw / 2 - 145, ctrlY + 65, 18, SKYBLUE);

	DrawText("Ibtisam Khan (CT-24023)",   sw / 2 - 145, ctrlY + 90, 16, WHITE);
	DrawText("Syed Wahaaj Ali (CT-24035)", sw / 2 - 145, ctrlY + 112, 16, WHITE);
	DrawText("Mashood Khan (CT-24021)",   sw / 2 - 145, ctrlY + 134, 16, WHITE);


	DrawText("Defeat the BOSS on Level 5!",
	         sw / 2 - 130, sh - 80, 18, Fade(MAGENTA, pulse));
}


void DrawVignette(float intensity, int sw, int sh) {
	if (intensity <= 0.01f) return;

	float spread = intensity * 0.5f;
	float leftWidth = sw * spread;
	float topHeight = sh * spread;

	DrawRectangleGradientV(0, 0, sw, topHeight, Fade(RED, intensity * 0.4f), Fade(RED, 0));
	DrawRectangleGradientV(0, sh - topHeight, sw, topHeight, Fade(RED, 0), Fade(RED, intensity * 0.4f));
	DrawRectangleGradientH(0, 0, leftWidth, sh, Fade(RED, intensity * 0.4f), Fade(RED, 0));
	DrawRectangleGradientH(sw - leftWidth, 0, leftWidth, sh, Fade(RED, 0), Fade(RED, intensity * 0.4f));
}

// Main Game
int main() {
	InitWindow(0, 0, "Escape The Maze - Boss Battle Edition");
	ToggleFullscreen();
	InitAudioDevice();
	SetTargetFPS(60);
	srand((unsigned)time(NULL));

	int sw = GetScreenWidth();
	int sh = GetScreenHeight();

	Texture2D grass = LoadTexture("grass.png");
	if (grass.id == 0) grass = LoadTexture("./assets/grass.png");
	if (grass.id == 0) grass = LoadTexture("C:/msys64/home/USER/grass.png");

	Music chaseMusic = LoadMusicStream("chase_music.mp3");
	Music bossMusic = LoadMusicStream("boss_music.mp3");
	Music levelMusic = LoadMusicStream("background_music.mp3");

	Sound caughtSound = LoadSound("caught.wav");
	Sound keySound = LoadSound("key.wav");
	Sound levelupSound = LoadSound("levelup.wav");
	Sound chestCollectSound = LoadSound("chest_collect.wav");

	SetMusicVolume(levelMusic, 0.7f);
	SetMusicVolume(chaseMusic, 0.7f);
	SetMusicVolume(bossMusic, 0.7f);

	Vector2 player = {(float)sw / 2, (float)sh / 2};
	float playerRadius = PLAYER_RADIUS;
	float playerSpeed = PLAYER_SPEED;
	float normalSpeed = playerSpeed;
	Color bushColor = {20, 80, 20, 255};
	Rectangle door = {(float)sw - 120, (float)sh / 2 - 40, 60, 80};

	int level = 1;
	bool isHidden = false;
	bool escaped = false;
	bool levelActive = true;
	bool bossBattleActive = false;
	float levelTimer = 60.0f;
	float enemyCountdown = 5.0f;
	float laserCountdown = 0.0f;
	Vector2 laserStart = {0, 0};
	Vector2 laserEnd = {0, 0};
	bool laserActive = false;
	float laserDuration = 0.5f;
	float laserActiveTimer = 0.0f;
	float redEffectIntensity = 0.0f;
	bool blackout = false;
	float minionSpawnTimer = 0.0f;

	int lives = 3;
	bool lastDeathOccurred = false;

	Chest chest;
	vector<Key> keys;
	vector<Bush> bushes;
	vector<Enemy> enemies;

	int collectedKeys = 0;
	int totalKeys = 0;
	float chestEffectTimer = 0.0f;
	bool inMenu = true;
	float gameTime = 0.0f;

	ResetLevel(player, enemies, keys, bushes, chest, collectedKeys, totalKeys,
	           enemyCountdown, levelTimer, levelActive, bossBattleActive,
	           minionSpawnTimer, normalSpeed, level, sw, sh, laserCountdown, laserActive);

	while (!WindowShouldClose()) {
		float delta = GetFrameTime();
		gameTime += delta;

		UpdateMusicStream(chaseMusic);
		UpdateMusicStream(bossMusic);
		UpdateMusicStream(levelMusic);

		if (inMenu) {
			BeginDrawing();
			DrawMenuScreen(sw, sh, gameTime);
			EndDrawing();
			if (IsKeyPressed(KEY_ENTER)) inMenu = false;
			continue;
		}

		if (enemyCountdown > 0) {
			enemyCountdown -= delta;
			if (enemyCountdown <= 0) {
				if (level == BOSS_LEVEL && !enemies.empty() && enemies[0].type == TYPE_BOSS) {
					enemies[0].released = true;
					enemies[0].pos.y = Clamp(enemies[0].pos.y, enemies[0].radius, (float)sh - enemies[0].radius);
					enemies[0].currentTarget = GetRandomFreeCell(sw, sh, bushes);
				} else {
					for (auto &e : enemies) e.released = true;
				}
			}
		}

		if (levelActive && enemyCountdown <= 0) {
			levelTimer -= delta;
			if (levelTimer <= 0) {
				levelActive = false;
				StopMusicStream(chaseMusic);
				StopMusicStream(bossMusic);
				StopMusicStream(levelMusic);
				StopSound(caughtSound);
				PlaySound(caughtSound);
				lives--;
				lastDeathOccurred = true;
			}
		}

		// Laser Attack Logic
		if (level == BOSS_LEVEL && levelActive && enemyCountdown <= 0.0f) {
			if (laserActive) {
				laserActiveTimer -= delta;
				if (laserActiveTimer <= 0.0f) {
					laserActive = false;
					laserCountdown = LASER_COOLDOWN;
				} else {
					float d = Vector2DistanceToLine(player, laserStart, laserEnd);
					if (d < playerRadius + 5.0f) {
						levelActive = false;
						StopMusicStream(chaseMusic);
						StopMusicStream(bossMusic);
						StopMusicStream(levelMusic);
						StopSound(caughtSound);
						PlaySound(caughtSound);
						lives--;
						lastDeathOccurred = true;
					}
				}
			} else {
				laserCountdown -= delta;
				if (laserCountdown <= 0.0f) {
					laserActive = true;
					laserActiveTimer = laserDuration;

					if (GetRandomValue(0, 1) == 0) {
						laserStart = {(float)GetRandomValue(0, sw), (float)sh * (GetRandomValue(0, 1))};
					} else {
						laserStart = {(float)sw * (GetRandomValue(0, 1)), (float)GetRandomValue(0, sh)};
					}

					if (laserStart.x == 0.0f) {
						laserEnd = {(float)sw, (float)GetRandomValue(0, sh)};
					} else if (laserStart.x == sw) {
						laserEnd = {0.0f, (float)GetRandomValue(0, sh)};
					} else if (laserStart.y == 0.0f) {
						laserEnd = {(float)GetRandomValue(0, sw), (float)sh};
					} else if (laserStart.y == sh) {
						laserEnd = {(float)GetRandomValue(0, sw), 0.0f};
					} else {
						laserEnd = RandomPoint(sw, sh);
					}

					if (Distance(laserStart, laserEnd) < 100.0f) {
						laserEnd = {(float)sw - laserEnd.x, (float)sh - laserEnd.y};
					}
				}
			}
		}

		Vector2 input = {0, 0};
		if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.x += 1;
		if (IsKeyDown(KEY_LEFT)  || IsKeyDown(KEY_A)) input.x -= 1;
		if (IsKeyDown(KEY_UP)    || IsKeyDown(KEY_W)) input.y -= 1;
		if (IsKeyDown(KEY_DOWN)  || IsKeyDown(KEY_S)) input.y += 1;

		if (input.x != 0.0f || input.y != 0.0f) {
			float len = sqrtf(input.x * input.x + input.y * input.y);
			input.x /= len;
			input.y /= len;
		}

		if (levelActive && !escaped) {
			Vector2 nextX = {player.x + input.x * playerSpeed * delta, player.y};
			if (!CircleCollidesAnyBush(nextX, playerRadius, bushes)) player.x = nextX.x;

			Vector2 nextY = {player.x, player.y + input.y * playerSpeed * delta};
			if (!CircleCollidesAnyBush(nextY, playerRadius, bushes)) player.y = nextY.y;

			player.x = Clamp(player.x, 0.0f, (float)sw);
			player.y = Clamp(player.y, 0.0f, (float)sh);
		}

		isHidden = false;
		for (auto &b : bushes) {
			if (CheckCollisionCircleRec(player, playerRadius, b.area)) {
				isHidden = true;
				break;
			}
		}

		for (auto &k : keys) {
			if (!k.collected && CheckCollisionPointCircle(player, k.pos, 20.0f)) {
				k.collected = true;
				collectedKeys++;
				StopSound(keySound);
				PlaySound(keySound);
			}
		}
		bool doorUnlocked = (collectedKeys == totalKeys);

		if (levelActive && !chest.opened && CheckCollisionPointCircle(player, chest.pos, 15.0f)) {
			chest.opened = true;
			chest.effect = (GetRandomValue(0, 1) == 0) ? 1 : 2;

			StopSound(chestCollectSound);
			PlaySound(chestCollectSound);

			if (chest.effect == 1) {
				playerSpeed *= 1.5f;
				chestEffectTimer = 5.0f;
			} else {
				int effectType = GetRandomValue(0, 4);
				if (effectType == 0) {
					levelTimer -= 10.0f;
				} else if (effectType == 1) {
					blackout = true;
					chestEffectTimer = 5.0f;
				} else if (effectType == 2) {
					for (auto &e : enemies) {
						if (e.type == TYPE_STANDARD) e.visionRange *= 2.0f;
					}
					chestEffectTimer = 5.0f;
				} else if (effectType == 3) {
					for (auto &e : enemies) {
						if (e.type == TYPE_STANDARD) e.speed *= 2.0f;
					}
					chestEffectTimer = 5.0f;
				} else if (effectType == 4) {
					auto newEnemies = CreateEnemies(2, sw, sh, bushes);
					for (auto &ne : newEnemies) ne.released = true;
					enemies.insert(enemies.end(), newEnemies.begin(), newEnemies.end());
					chestEffectTimer = 5.0f;
				}
			}
		}

		if (chestEffectTimer > 0.0f) {
			chestEffectTimer -= delta;
			if (chestEffectTimer <= 0.0f) {
				playerSpeed = normalSpeed;
				blackout = false;
				for (auto &e : enemies) {
					if (e.type == TYPE_STANDARD) {
						e.visionRange = 200.0f;
						e.speed = 130.0f + GetRandomValue(0, 30);
					}
				}
			}
		}

		if (level == BOSS_LEVEL && enemyCountdown <= 0.0f) {
			minionSpawnTimer -= delta;
			if (minionSpawnTimer <= 0.0f) {
				minionSpawnTimer = 3.0f;
				for (int i = 0; i < 2; i++) {
					Enemy minion;
					minion.type = TYPE_MINION;
					minion.radius = 10.0f;
					minion.speed = 200.0f;
					minion.pos = RandomPoint(sw, sh);
					minion.released = true;
					minion.hasTarget = false;
					enemies.push_back(minion);
				}
			}
		}

		if (level == BOSS_LEVEL && !enemies.empty() && enemies[0].type == TYPE_BOSS && enemies[0].released) {
			Enemy &boss = enemies[0];
			if (Distance(boss.pos, boss.currentTarget) < 20.0f || GetRandomValue(0, 30) < 1) {
				boss.currentTarget = GetRandomFreeCell(sw, sh, bushes);
				boss.moveDirection = Vector2Normalize({boss.currentTarget.x - boss.pos.x, boss.currentTarget.y - boss.pos.y});
			}
			boss.pos.x += boss.moveDirection.x * boss.speed * delta;
			boss.pos.y += boss.moveDirection.y * boss.speed * delta;
			boss.pos.x = Clamp(boss.pos.x, boss.radius, (float)sw - boss.radius);
			boss.pos.y = Clamp(boss.pos.y, boss.radius, (float)sh - boss.radius);
		}

		bool anyChasing = false;
		if (enemyCountdown <= 0.0f && !escaped && levelActive) {
			for (auto &e : enemies) {
				if (e.type == TYPE_STANDARD) {
					e.visionRange += e.visionGrowRate * delta;
					float dist = Distance(e.pos, player);
					bool chasingNow = (e.released && !isHidden && dist < e.visionRange);

					if (chasingNow && dist > 0.0001f) {
						anyChasing = true;
						Vector2 nextTarget = EnemyNextTargetOnGrid(e, player, sw, sh, bushes);
						Vector2 dir = {nextTarget.x - e.pos.x, nextTarget.y - e.pos.y};
						float llen = sqrtf(dir.x*dir.x + dir.y*dir.y);

						if (llen > 5.0f) {
							dir.x /= llen;
							dir.y /= llen;
							float step = e.speed * delta;
							Vector2 proposed = {e.pos.x + dir.x * step, e.pos.y + dir.y * step};

							if (!CircleCollidesAnyBush(proposed, e.radius, bushes)) {
								e.pos = proposed;
							} else {
								Vector2 propX = {e.pos.x + dir.x * step, e.pos.y};
								Vector2 propY = {e.pos.x, e.pos.y + dir.y * step};
								if (!CircleCollidesAnyBush(propX, e.radius, bushes)) {
									e.pos = propX;
								} else if (!CircleCollidesAnyBush(propY, e.radius, bushes)) {
									e.pos = propY;
								}
							}
						}
					} else if (e.released) {
						if (!e.hasTarget || Distance(e.pos, e.currentTarget) < 20.0f) {
							e.currentTarget = GetRandomFreeCell(sw, sh, bushes);
							e.hasTarget = true;
						}
						Vector2 nextTarget = EnemyNextTargetOnGrid(e, e.currentTarget, sw, sh, bushes);
						Vector2 dir = {nextTarget.x - e.pos.x, nextTarget.y - e.pos.y};
						float len = sqrtf(dir.x * dir.x + dir.y * dir.y);

						if (len > 5.0f) {
							dir.x /= len;
							dir.y /= len;
							float step = e.speed * delta * 0.7f;
							Vector2 proposed = {e.pos.x + dir.x * step, e.pos.y + dir.y * step};

							if (!CircleCollidesAnyBush(proposed, e.radius, bushes)) {
								e.pos = proposed;
							} else {
								Vector2 propX = {e.pos.x + dir.x * step, e.pos.y};
								Vector2 propY = {e.pos.x, e.pos.y + dir.y * step};
								if (!CircleCollidesAnyBush(propX, e.radius, bushes)) {
									e.pos = propX;
								} else if (!CircleCollidesAnyBush(propY, e.radius, bushes)) {
									e.pos = propY;
								} else {
									e.hasTarget = false;
								}
							}
						}
					}
				} else if (e.type == TYPE_MINION) {
					if (!e.hasTarget || Distance(e.pos, e.currentTarget) < 20.0f) {
						e.currentTarget = GetRandomFreeCell(sw, sh, bushes);
						e.moveDirection = Vector2Normalize({e.currentTarget.x - e.pos.x, e.currentTarget.y - e.pos.y});
						e.hasTarget = true;
					}
					Vector2 nextPos = {e.pos.x + e.moveDirection.x * e.speed * delta, e.pos.y + e.moveDirection.y * e.speed * delta};
					e.pos = nextPos;
					e.pos.x = Clamp(e.pos.x, e.radius, (float)sw - e.radius);
					e.pos.y = Clamp(e.pos.y, e.radius, (float)sh - e.radius);
				}

				if (e.type == TYPE_BOSS && e.released) anyChasing = true;
				if (e.type == TYPE_MINION && e.released) anyChasing = true;

				if (CheckCollisionCircles(player, playerRadius, e.pos, e.radius)) {
					levelActive = false;
					StopMusicStream(chaseMusic);
					StopMusicStream(bossMusic);
					StopMusicStream(levelMusic);
					StopSound(caughtSound);
					PlaySound(caughtSound);
					lives--;
					lastDeathOccurred = true;
				}
			}
		}

		float targetIntensity = anyChasing ? GetChaseIntensity(enemies, player) : 0.0f;
		float fadeSpeed = 2.0f;
		if (redEffectIntensity < targetIntensity) {
			redEffectIntensity += fadeSpeed * delta;
		} else {
			redEffectIntensity -= fadeSpeed * delta;
		}
		redEffectIntensity = Clamp(redEffectIntensity, 0.0f, 1.0f);

		// Music Management
		bool isBossActive = (level == BOSS_LEVEL && bossBattleActive && enemyCountdown <= 0.0f);
		bool shouldPlayChase = anyChasing && levelActive && !escaped;

		if (levelActive && !escaped) {
			if (isBossActive) {
				if (shouldPlayChase) {
					if (!IsMusicStreamPlaying(bossMusic)) {
						StopMusicStream(chaseMusic);
						StopMusicStream(levelMusic);
						PlayMusicStream(bossMusic);
					}
				} else {
					if (IsMusicStreamPlaying(bossMusic)) StopMusicStream(bossMusic);
				}
			} else {
				if (shouldPlayChase) {
					if (!IsMusicStreamPlaying(chaseMusic)) {
						StopMusicStream(bossMusic);
						StopMusicStream(levelMusic);
						PlayMusicStream(chaseMusic);
					}
				} else {
					if (!IsMusicStreamPlaying(levelMusic)) {
						StopMusicStream(bossMusic);
						StopMusicStream(chaseMusic);
						PlayMusicStream(levelMusic);
					}
				}
			}
		} else {
			if (IsMusicStreamPlaying(chaseMusic)) StopMusicStream(chaseMusic);
			if (IsMusicStreamPlaying(bossMusic)) StopMusicStream(bossMusic);
			if (IsMusicStreamPlaying(levelMusic)) StopMusicStream(levelMusic);
		}

		if (doorUnlocked && CheckCollisionPointRec(player, door)) {
			if (level == BOSS_LEVEL) {
				escaped = true;
			} else {
				level++;
				StopSound(levelupSound);
				PlaySound(levelupSound);
				StopMusicStream(chaseMusic);
				StopMusicStream(bossMusic);
				StopMusicStream(levelMusic);
				ResetLevel(player, enemies, keys, bushes, chest, collectedKeys, totalKeys,
				           enemyCountdown, levelTimer, levelActive, bossBattleActive,
				           minionSpawnTimer, normalSpeed, level, sw, sh, laserCountdown, laserActive);
			}
		}

		BeginDrawing();

		if (!blackout) {
			if (grass.id != 0) {
				for (int y = 0; y < sh; y += grass.height) {
					for (int x = 0; x < sw; x += grass.width) {
						DrawTexture(grass, x, y, WHITE);
					}
				}
			} else {
				ClearBackground(DARKGREEN);
			}
		} else {
			ClearBackground(BLACK);
		}

		DrawPlayer(player, playerRadius, isHidden, gameTime);

		if (!blackout) {
			for (auto &b : bushes) {
				DrawRectangleRec(b.area, bushColor);
				DrawRectangleLinesEx(b.area, 2, Fade(BLACK, 0.5f));
			}
		}

		for (auto &k : keys) {
			if (!k.collected) DrawKey(k.pos, gameTime);
		}

		DrawExitDoor(door, doorUnlocked, gameTime);

		if (!chest.opened) {
			DrawRectangleV(chest.pos, {24.0f, 24.0f}, PURPLE);
			Rectangle chestRect = {chest.pos.x, chest.pos.y, 24, 24};
			DrawRectangleLinesEx(chestRect, 2, VIOLET);

			float chestGlow = 0.5f + 0.5f * sinf(gameTime * 3.0f);
			Rectangle chestGlowRect = {chest.pos.x - 2, chest.pos.y - 2, 28, 28};
			DrawRectangleLinesEx(chestGlowRect, 1, Fade(VIOLET, chestGlow));
		}

		for (auto &e : enemies) {
			if (e.type == TYPE_STANDARD && e.released && !blackout) {
				DrawCircleLines(e.pos.x, e.pos.y, e.visionRange, Fade((Color) {
					80, 0, 0, 150
				}, 0.5f));
			}
			DrawEnemy(e, gameTime, enemyCountdown);
		}

		if (level == BOSS_LEVEL && laserCountdown > 0.0f && enemyCountdown <= 0.0f) {
			DrawText(TextFormat("LASER: %.1f", laserCountdown), sw / 2 - 50, sh - 40, 20, YELLOW);
		}

		if (laserActive) {
			Color laserColor = (laserActiveTimer > laserDuration * 0.5f) ? RED : Fade(RED, 0.8f);
			float thickness = 10.0f;
			if (laserActiveTimer > laserDuration * 0.8f) {
				thickness = 10.0f + 5.0f * sinf(gameTime * 20.0f);
			}
			DrawLineEx(laserStart, laserEnd, thickness, laserColor);
		}

		DrawStyledHUD(level, collectedKeys, totalKeys, lives, levelTimer, levelActive, enemyCountdown, sw, blackout);

		if (!levelActive && !escaped) {
			DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.7f));
			DrawText("GAME OVER!", sw / 2 - 150, sh / 2, 40, RED);

			if (lives > 0) {
				DrawText(TextFormat("You have %d lives left", lives), sw/2 - 120, sh/2 + 40, 20, WHITE);
				DrawText("Press ENTER to Retry this Level", sw / 2 - 210, sh / 2 + 80, 20, WHITE);
			} else {
				DrawText("You have no lives left. Will reset to Level 1.", sw/2 - 220, sh/2 + 40, 20, WHITE);
				DrawText("Press ENTER to Restart from Level 1", sw / 2 - 210, sh / 2 + 80, 20, WHITE);
			}

			if (IsKeyPressed(KEY_ENTER)) {
				if (lives <= 0) {
					level = 1;
					lives = 3;
					escaped = false;
				}
				StopMusicStream(chaseMusic);
				StopMusicStream(bossMusic);
				StopMusicStream(levelMusic);
				ResetLevel(player, enemies, keys, bushes, chest, collectedKeys, totalKeys,
				           enemyCountdown, levelTimer, levelActive, bossBattleActive,
				           minionSpawnTimer, normalSpeed, level, sw, sh, laserCountdown, laserActive);
				lastDeathOccurred = false;
			}
		}

		if (escaped) {
			DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.8f));
			float pulse = 0.8f + 0.2f * sinf(gameTime * 3.0f);
			DrawText("VICTORY!", sw / 2 - 180, sh / 2 - 80, 60, Fade(GOLD, pulse));
			DrawText("YOU DEFEATED THE BOSS!", sw / 2 - 240, sh / 2 - 10, 40, LIME);
			DrawText("All 10 keys collected and escaped the final maze!", sw / 2 - 280, sh / 2 + 50, 20, WHITE);
			DrawText("Press ESC to quit", sw / 2 - 120, sh / 2 + 100, 20, YELLOW);
		}

		DrawVignette(redEffectIntensity, sw, sh);
		EndDrawing();
	}

	if (grass.id != 0) UnloadTexture(grass);
	UnloadMusicStream(chaseMusic);
	UnloadMusicStream(bossMusic);
	UnloadMusicStream(levelMusic);
	UnloadSound(caughtSound);
	UnloadSound(keySound);
	UnloadSound(levelupSound);
	UnloadSound(chestCollectSound);
	CloseAudioDevice();
	CloseWindow();
	return 0;
}