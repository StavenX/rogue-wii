#include <string>
#include <algorithm>

void game_loop();
void draw_loop();
void handleProjectileCollisions();
void removeExpiredProjectiles();
void addEntity();
void console();

int numParticles = 0; 
bool paused = false;
int seed = rand();

Area area(seed);

EnemySpawner* enemySpawner = new EnemySpawner();

/* Happens just once before other game loops */
void setup() {
}
 
/* All logic-events */
void game_loop() {
    PAD_ScanPads();
    u16 pressed = PAD_ButtonsDown(0);
    if(BUTTON_B) {
        paused = !paused;        
    }

    if(!paused) {
        /* Refresh input from controller */
        numParticles = projectiles.size();
        area.update(player.getX(), player.getY());

        numParticles = entities.size();

        /* All entities will act */
        for (Entity* entity : entities) {
            entity->act();
        }

        for (Projectile * p : projectiles) {
            p->act();
        }
        /* Tick down spawner */
        enemySpawner->tickDown();
        
        handleProjectileCollisions();
        removeExpiredProjectiles();
    }
}

/* All draw-events */
void draw_loop() {
    Gui gui(camera.x, camera.y);
    if(!paused) {
        area.draw();

        /* Draw all entities currently in the "scene" */
        for (Entity* entity : entities) {
            entity->draw();
        }
        for (Projectile* p : projectiles) {
            p->draw();
        }
        gui.draw_dashboard(80);
        gui.draw_text(to_string(entities.size()), 10, 10, TEXT_MEDIUM);
    } else {
        gui.draw_dashboard(500);
        gui.draw_text("PAUSED", 160, SCREEN_HEIGHT / 2 - 32, TEXT_BIG);
    }
}

/* Initiate console-mode */
void console() {
    // console setup stuff
    VIDEO_Init();
    PAD_Init();
    rmode = VIDEO_GetPreferredMode(NULL);
    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
    console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
    VIDEO_Configure(rmode);
    VIDEO_SetNextFramebuffer(xfb);
    VIDEO_SetBlack(FALSE);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

    // ------------------------------------------------------------------
    // PRINT STUFF HERE
    printf("\n\n"); // it starts printing way up, idk
    printf("Hello World!\n");
    // ------------------------------------------------------------------

    while(true) {
        PAD_ScanPads();
        u32 pressed = PAD_ButtonsDown(0);
        if (BUTTON_START) exit(0);
        VIDEO_WaitVSync();
    }
}

/* Removes projectiles that are have been marked "dead" (not in use anymore) */
void removeExpiredProjectiles() {
    auto lambda = [](Projectile* p){

        if (p->isDead() || p->markedForDeletion) {
            delete(p);
            return true;
        }
        return false;
    };
    auto it = std::remove_if(projectiles.begin(), projectiles.end(), lambda);
    projectiles.erase(it, projectiles.end());
}

/* Helper method for adding entities to the game */
void addEntity(Entity* entity) {
	entities.push_back(entity);
}

/* Takes care of all projectile-based collisions */
void handleProjectileCollisions() {

    for (Entity* e : entities) {
        for (Projectile* p : projectiles) {
            if (p->sprite.isColliding(e->sprite)    // Is actually colliding with the object
                && p->projectileOwner != e) {       // And cannot collide with its own owner
                
                p->markedForDeletion = true;
                e->handleProjectile(p);
            } 
        }
    }
}

