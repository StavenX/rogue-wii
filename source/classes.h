#include <vector>

#define WIDTH 64
#define HEIGHT 64

class TexCoord;
class Sprite;
class Entity;
class Projectile;
class Player;
class Chunk;
class Area;
class Enemy;
class Text;
class Camera;
class Spawner;
class EnemySpawner;

/* All possible directions an entity can have */
enum Direction {
    LEFT, RIGHT
};

class TexCoord {
public:
    tuple<double, double> topleft;
    tuple<double, double> topright;
    tuple<double, double> bottomright;
    tuple<double, double> bottomleft;
    /// Returns coordinates for where the image is located in memory
    /// by the i'th and j'th position granted.
    TexCoord(int i, int j) {
        double x = (double)i;
        double y = (double)j;
        double dx = 1.0 / (IMAGE_WIDTH / WIDTH);
        double dy = 1.0 / (IMAGE_HEIGHT / HEIGHT);
        this->topleft = make_tuple(x * dx, y * dy);
        this->topright = make_tuple(x * dx + dx, y * dy);
        this->bottomright = make_tuple(x * dx + dx, y * dy + dy);
        this->bottomleft = make_tuple(x * dx, y * dy + dy);
    }
};

class Sprite {
public:
    int x;
    int y;
    int width;
    int height;
    int i;
    int j;
    Sprite() {
        this->x = 0;
        this->y = 0;
        this->width = WIDTH;
        this->height = HEIGHT;
        this->i = 0;
        this->j = 0;
    }
    Sprite(int x, int y, int width, int height, int i, int j) {
        this->x = x;
        this->y = y;
        this->i = i;
        this->j = j;
        this->width = width;
        this->height = height;
    }
    void set_texcoord(int i, int j) {
        this->i = i;
        this->j = j;
    }
    bool isColliding(Sprite other) {
        int x1 = this->x;
        int x2 = other.x;
        int y1 = this->y;
        int y2 = other.y;
        int w1 = this->width;
        int w2 = other.width;
        int h1 = this->height;
        int h2 = other.height;

        if (x1 < x2 + w2 &&
                x1 + w1 > x2 &&
                y1 < y2 + h2 &&
                y1 + h1 > y2) {
            return true;
        }

        return false;
    }
    void draw() {
        int x = this->x;
        int y = this->y;
        int width = this->width;
        int height = this->height;
        TexCoord coord(this->i, this->j);

        GX_Begin(GX_QUADS, GX_VTXFMT0, 4);
        GX_Position2f32(x, y);
        GX_TexCoord2f32(get<0>(coord.topleft), get<1>(coord.topleft));
        GX_Position2f32(x + width - 1, y);
        GX_TexCoord2f32(get<0>(coord.topright), get<1>(coord.topright));
        GX_Position2f32(x + width - 1, y + height - 1);
        GX_TexCoord2f32(get<0>(coord.bottomright), get<1>(coord.bottomright));
        GX_Position2f32(x, y + height - 1);
        GX_TexCoord2f32(get<0>(coord.bottomleft), get<1>(coord.bottomleft));
        GX_End();
    }
};

/* Superclass for things that can be drawn and does something */
class Entity {
	public:
		Sprite sprite;

        virtual ~Entity() {}
		void draw() {
			this->sprite.draw();
		}
		int getX() {
			return this->sprite.x;
		}
		int getY() {
			return this->sprite.y;
		}

        void move(int x, int y) {
            this->sprite.x += x;
            this->sprite.y += y;
        }

        /* What's supposed to happen when entity gets hit by projectiles */
        virtual void handleProjectile(Projectile* p) {}

		/* Implement this in each derived entity */
		virtual void act() {}
};

class Projectile : public Entity {
    public: 
        Direction direction = RIGHT; // by default
        int speed = 1;
        int travelDistance = 0;
        int maxTravelDistance = 100;

        bool markedForDeletion = false; 
        Entity* projectileOwner; 

    Projectile() {
        this->sprite = Sprite(100, 480 / 2, 64, 64, FLAME_SPRITE);
    }

    Projectile(Direction d, int x, int y) : Projectile() {
        this->sprite.x = x;
        this->sprite.y = y; 
        this->direction = d;
    }

    void act() {

        if (this->direction == RIGHT) {
            move(speed, 0);
        } else if (this->direction == LEFT) {
            move(-speed, 0);
        }
        travelDistance += speed;
    }

    bool isDead() {
        if (maxTravelDistance > travelDistance) return false;
        else return true;
    }
};

/* Stores all projectiles currently in the game */
vector<Projectile*> projectiles;

class Player: public Entity {
	public:
		int health;
		int damage;
		int xp;
		int animation_timer;
		double speed; 
        
        /* Attacking logic */
        int attackTimer;
        int attackSpeed = 30; 

		Direction direction = Direction::RIGHT;

		Player() {
			this->sprite = Sprite(100, 480 / 2, 64, 64, PLAYER_RIGHT_SPRITE);
			this->health = 10;
			this->damage = 5;
			this->xp = 0;
			this->animation_timer = 0;
			this->speed = 3.0;
            this->attackTimer = 0;
		}

        /* Handle attack-speed */
        bool canAttack() {
            if (attackTimer <= 0) {
                return true;
            } else {
                return false;
            }
        }
		void resetAttackTimer() {
            this->attackTimer = attackSpeed;
        }
        void decreaseAttackTimer() {
            this->attackTimer--;
        }
        
        void act() {
			// Has to be declared each frame AND right here
			u16 pressed = PAD_ButtonsHeld(0);

			if (BUTTON_START) exit(0);

            if (!canAttack()) {
                decreaseAttackTimer();
            }

			double dx = STICK_X / 64.0;
			double dy = STICK_Y / 64.0;
            
            if (BUTTON_A && pressed && canAttack()) {
                Projectile* to_shoot = new Projectile(this->direction, getX(), getY());
                to_shoot->projectileOwner = this; 
                projectiles.push_back(to_shoot);
                resetAttackTimer();
            }


			bool move = false;
			if(abs(dx) > 0.07) {
				move = true;
				if(dx >= 0) {
					direction = Direction::RIGHT;
				} else {
					direction = Direction::LEFT;
				}
				sprite.x += (int)(dx * speed);
			}
			if(abs(dy) > 0.07) {
				move = true;
				sprite.y -= (int)(dy * speed);
			}
			if(move) {
				animation_timer = (animation_timer + 1) % 20;
			}

			switch(direction) {
				case Direction::LEFT:
					if(animation_timer < 10) {
						sprite.set_texcoord(PLAYER_LEFT_SPRITE);
					} else {
						sprite.set_texcoord(PLAYER_LEFT_WALK_SPRITE);
					}
					break;
				case Direction::RIGHT:
					if(animation_timer < 10) {
						sprite.set_texcoord(PLAYER_RIGHT_SPRITE);
					} else {
						sprite.set_texcoord(PLAYER_RIGHT_WALK_SPRITE);
					}
					break;
			}
		}
};

#define CHUNK_SIZE 6
#define CHUNK_SPACING 64 * CHUNK_SIZE
class Chunk {
public:
    Sprite blocks[CHUNK_SIZE][CHUNK_SIZE];
    int origin_x;
    int origin_y;
    int seed;
    Chunk() {
        origin_x = 0;
        origin_y = 0;
        seed = 0;
    }
    Chunk(int origin_x, int origin_y, int seed) {
        this->seed = seed;
        this->origin_x = origin_x;
        this->origin_y = origin_y;
        FastNoiseLite noise(this->seed);
        noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
        for(int i = 0; i < CHUNK_SIZE; i++) {
            for(int j = 0; j < CHUNK_SIZE; j++) {
                this->blocks[i][j].x = this->origin_x * CHUNK_SPACING + i * 64;
                this->blocks[i][j].y = this->origin_y * CHUNK_SPACING + j * 64;
                float value = noise.GetNoise((float)(origin_x * CHUNK_SIZE + i), (float)(origin_y * CHUNK_SIZE + j));
                if(value > -0.25) {
                    this->blocks[i][j].set_texcoord(GRASS_SPRITE);
                } else if(value > -0.35) {
                    this->blocks[i][j].set_texcoord(STONE_SPRITE);
                } else {
                    this->blocks[i][j].set_texcoord(WATER_SPRITE);
                }

            }
        }
    }
    void draw() {
        for(int i = 0; i < CHUNK_SIZE; i++) {
            for(int j = 0; j < CHUNK_SIZE; j++) {
                this->blocks[i][j].draw();
            }
        }
    }
};

class Area {
    public:
        int seed;
        tuple<int, int> bounds_x;
        tuple<int, int> bounds_y;
        vector<vector<Chunk>> chunks;
        Area(int seed) {
            this->seed = seed;
            for(int j = 0; j < 3; j++) {
                vector<Chunk> temp_chunks;
                for(int i = 0; i < 3; i++) {
                    Chunk chunk(i, j, this->seed);
                    temp_chunks.push_back(chunk);
                }
                this->chunks.push_back(temp_chunks);
            }
            int temp_x = this->chunks[1][1].origin_x * CHUNK_SPACING;
            int temp_y = this->chunks[1][1].origin_y * CHUNK_SPACING;
            this->bounds_x = make_tuple(temp_x, temp_x + CHUNK_SPACING);
            this->bounds_y = make_tuple(temp_y, temp_y + CHUNK_SPACING);

        }
        void update(int x, int y) {
            bool new_bounds = false;
            if((x < get<0>(bounds_x)) && this->chunks[0][0].origin_x > 0) {
                new_bounds = true;
                for(int i = 0; i < 3; i++) {
                    this->chunks[i].erase(this->chunks[i].begin() + 2);
                    int origin_x = this->chunks[i][0].origin_x;
                    int origin_y = this->chunks[i][0].origin_y;
                    Chunk chunk(origin_x - 1, origin_y, this->seed);
                    this->chunks[i].insert(this->chunks[i].begin(), chunk);
                }
            } else if(x > get<1>(bounds_x)) {
                new_bounds = true;
                for(int i = 0; i < 3; i++) {
                    this->chunks[i].erase(this->chunks[i].begin());
                    int origin_x = this->chunks[i][1].origin_x;
                    int origin_y = this->chunks[i][1].origin_y;
                    Chunk chunk(origin_x + 1, origin_y, this->seed);
                    this->chunks[i].push_back(chunk);
                }
            } else if((y < get<0>(bounds_y)) && this->chunks[0][0].origin_y > 0) {
                new_bounds = true;
                this->chunks.erase(this->chunks.begin() + 2);
                vector<Chunk> temp_chunks;
                for(int i = 0; i < 3; i++) {
                    int origin_x = this->chunks[0][i].origin_x;
                    int origin_y = this->chunks[0][i].origin_y;
                    Chunk chunk(origin_x, origin_y - 1, this->seed);
                    temp_chunks.push_back(chunk);
                }
                this->chunks.insert(this->chunks.begin(), temp_chunks);
            } else if(y > get<1>(bounds_y)) {
                new_bounds = true;
                this->chunks.erase(this->chunks.begin());
                vector<Chunk> temp_chunks;
                for(int i = 0; i < 3; i++) {
                    int origin_x = this->chunks[1][i].origin_x;
                    int origin_y = this->chunks[1][i].origin_y;
                    Chunk chunk(origin_x, origin_y + 1, this->seed);
                    temp_chunks.push_back(chunk);
                }
                this->chunks.push_back(temp_chunks);
            }
            if(new_bounds) {
                int temp_x = this->chunks[1][1].origin_x * CHUNK_SPACING;
                int temp_y = this->chunks[1][1].origin_y * CHUNK_SPACING;
                this->bounds_x = make_tuple(temp_x, temp_x + CHUNK_SPACING);
                this->bounds_y = make_tuple(temp_y, temp_y + CHUNK_SPACING);
            }
        }
        void draw() {
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    this->chunks[i][j].draw();
                }
            }
        }
};

class Enemy: public Entity {
	public:
		Enemy() {
			this->sprite = Sprite(rand() % 500, rand() % 500, 64, 64, ENEMY_SPRITE);
		}

        void act() {
            this->sprite.x += (5 - rand() % 11);
            this->sprite.y += (5 - rand() % 11);
        }

		void act(Player player) {

			// horizontal
			if (this->sprite.x < player.sprite.x) {
				this->sprite.x++;
			} else if (this->sprite.x > player.sprite.x) {
				this->sprite.x--;
			}

			// vertical
			if (this->sprite.y < player.sprite.y) {
				this->sprite.y++;
			} else if (this->sprite.y > player.sprite.y) {
				this->sprite.y--;
			}
		}
};

#define TEXT_BIG 64
#define TEXT_MEDIUM 48
#define TEXT_SMALL 32
class Text {
public:
    vector<Sprite> letters;
    Text(string text, int x, int y, int size) {
        int index = 0;
        int spacing = (int)(size * 0.8);
        // this loops over each letter and creates a sprite
        // based on the letter ascii value and translates its value
        // into coordinate in spritesheet
        for (char letter: text) {
            int ascii = (int)letter;
            int i = 0;
            int j = 0;
            // if it's digit
            if(ascii <= '9') {
                // if it's space
                if(ascii == ' ') {
                    i = 12;
                    j = 3;
                } else {
                    i = ascii - 48;
                }
                // otherwise letters
            } else {
                j++;
                if(ascii > 'M') {
                    j++;
                }
                i = (ascii - 'A') % 13;
            }
            Sprite sprite = Sprite(x + index * spacing, y, size, size, i, j);
            this->letters.push_back(sprite);
            index++;
        }
    }
    void draw() {
        for (Sprite &letter: this->letters) {
            letter.draw();
        }
    }
};

class Camera {
    public:
        int x;
        int y;
        int smoothing;
        Camera() {
            this->x = 0;
            this->y = 0;
            this->smoothing = 20;
        }
        void follow_smooth(int object_x, int object_y) {
            this->x += (object_x - (this->x + (SCREEN_WIDTH - 64) / 2)) / this->smoothing;
            this->y += (object_y - (this->y + (SCREEN_HEIGHT - 64) / 2)) / this->smoothing;
            if(this->x < 0) this->x = 0;
            if(this->y < 0) this->y = 0;
        }
};

class Gui {
    public:
        int camera_x;
        int camera_y;
        Gui(int camera_x, int camera_y) {
            this->camera_x = camera_x;
            this->camera_y = camera_y;
        }
        void draw_dashboard(int height) {
            Sprite dashboard = Sprite(camera_x - 10, camera_y - 10, SCREEN_WIDTH + 50, height, WHITE_SPRITE);
            dashboard.draw();
        }
        void draw_text(string text, int offset_x, int offset_y, int size) {
            Text object(text, this->camera_x + offset_x, this->camera_y + offset_y, size);
            object.draw();
        }
};

Player player;

/* Holds ALL entities currently in the scene */
vector<Entity*> entities { &player };

class EnemySpawner {
    public:
        int timer = 60; // one second
        int time = 0;  

        EnemySpawner() {}

        EnemySpawner(int maxTimer, int startTime) {
            this->timer = maxTimer;
            this->time = startTime;
        }

        void spawn() {
            // Projectile* to_shoot = new Projectile(this->direction, getX(), getY());
            Enemy* to_spawn = new Enemy();
            entities.push_back(to_spawn);
        }

        void resetTimer() {
            this->time = timer;
        }

        void tickDown() {
            if (this->time <= 0) {
                resetTimer();
                spawn();
            } else {
                this->time--;
            }
        }
};
