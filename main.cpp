#pragma comment (linker, "/subsystem:console")

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "SDL2-2.0.9/include/SDL.h"
#pragma comment(lib, "SDL2-2.0.9\\lib\\x86\\SDL2.lib")
#pragma comment(lib, "SDL2-2.0.9\\lib\\x86\\SDL2main.lib")
#include "SDL2-2.0.9/include/SDL_image.h"
#pragma comment (lib, "SDL2-2.0.9\\lib\\x86\\SDL2_image.lib")
#pragma warning(disable:4996)

void load_Image_To_Texture(SDL_Texture** dest, SDL_Renderer* renderer, const char* src)
{
	SDL_Surface* temp = IMG_Load(src);
	assert(temp);
	*dest = SDL_CreateTextureFromSurface(renderer, temp);
	assert(*dest);
	SDL_FreeSurface(temp);
}

void load_Image_And_Size_To_Texture(SDL_Texture** dest, int* width, int* height, SDL_Renderer* renderer, const char* src)
{
	SDL_Surface* temp = IMG_Load(src);
	assert(temp);
	SDL_LockSurface(temp);
	*width = temp->w;
	*height = temp->h;
	SDL_UnlockSurface(temp);
	*dest = SDL_CreateTextureFromSurface(renderer, temp);
	assert(*dest);
	SDL_FreeSurface(temp);
}

namespace Text
{
	struct Font
	{
		SDL_Texture* src;
		int font_w, font_h, src_w, src_h, n_cols;
	};

	char* int_To_String(int input)
	{
		char negative = 0;
		int digit_count = 0;
		int input_copy = input;
		if (input < 0)
		{
			negative = 1;
			input_copy *= -1;
		}

		while (input_copy > 0)
		{
			digit_count++;
			input_copy /= 10;
		}
		if (digit_count == 0) return (char*)"0";

		int pos = 0;
		char* num_string;
		if (negative == 0) num_string = (char*)calloc(digit_count + 1, sizeof(char));
		else
		{
			num_string = (char*)calloc(digit_count + 2, sizeof(char));
			num_string[0] = '-';
			pos++;
			input *= -1;
		}
		for (int i = 1; i <= digit_count; i++)
		{
			int factor = digit_count - i;
			int divisor = 1;
			for (int j = 0; j < factor; j++) divisor *= 10;
			num_string[pos] = ((input / divisor) % 10) + 48;
			pos++;
		}
		return num_string;
	}

	void draw_Text(SDL_Renderer* renderer, const char* input, int box_x, int box_y, int box_w, Font* font, int size)
	{
		SDL_Rect dest;
		dest.x = box_x;
		dest.y = box_y;
		dest.w = size;
		dest.h = (int)(size * 1.5);

		SDL_Rect src;
		src.w = font->font_w;
		src.h = font->font_h;

		int n_data = strlen(input);
		for (int i = 0; i < n_data; i++)
		{
			src.x = src.w * (input[i] % font->n_cols);
			src.y = src.h * (input[i] / font->n_cols);

			SDL_RenderCopyEx(renderer, font->src, &src, &dest, 0, NULL, SDL_FLIP_NONE);
			dest.x += dest.w;
			if (dest.x + dest.w > box_w)
			{
				dest.x = box_x;
				dest.y += size;
			}
		}
	}

	void draw_Int(SDL_Renderer* renderer, int input, int box_x, int box_y, int box_w, Font* font, int size)
	{
		//convert integer to string, then print string
		draw_Text(renderer, (const char*)int_To_String(input), box_x, box_y, box_w, font, size);
	}
}

//tangent rooms and direction entity is facing
enum { MID = 0, LEFT, RIGHT, DOWN, UP };

struct Sheet
{
	int sprite_w, sprite_h;
	int sheet_w, sheet_h, n_cols;
	SDL_Texture* sheet;
};

void load_Sheet(Sheet* dest, int grid_w, int grid_h, SDL_Renderer* renderer, const char* src)
{
	load_Image_And_Size_To_Texture(&dest->sheet, &dest->sheet_w, &dest->sheet_h, renderer, src);
	dest->sprite_w = grid_w;
	dest->sprite_h = grid_h;
	dest->n_cols = dest->sheet_w / grid_w;
}

struct Wall_Set
{
	int* visual;
	int* collision;
};

struct Floor_Set
{
	int* floor;
	int* props;
	int* spawns;
	int* collision;
};

struct Map_Key
{
	int room_w, room_h, n_walls, n_floors;
	Wall_Set* walls;
	Floor_Set* floors;
};

void load_CSV(int* dest, int width, int height, const char* src)
{
	FILE* f_in = fopen(src, "r");
	assert(f_in != NULL);
	for (int i = 0; i < height; i++)
	{
		fscanf(f_in, "%d", &dest[i * width]);
		for (int j = 1; j < width; j++)
		{
			fscanf(f_in, ",%d", &dest[i * width + j]);
		}
		fscanf(f_in, "\n");
	}
	fclose(f_in);
}

void load_Map_Key(Map_Key* key, int room_w, int room_h, int n_walls, int n_floors)
{
	int room_area = room_w * room_h;
	key->room_w = room_w;
	key->room_h = room_h;
	key->n_walls = n_walls;
	key->n_floors = n_floors;

	char* path = (char*)calloc(64, sizeof(char));

	//store as sets so correct collisions for parts, 
	//	but can swap walls and floors as needed for layout
	key->walls = (Wall_Set*)calloc(n_walls, sizeof(Wall_Set));
	for (int i = 0; i < n_walls; ++i)
	{
		key->walls[i].visual = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Demo_Wall %d.csv", i);
		load_CSV(key->walls[i].visual, room_w, room_h, path);

		key->walls[i].collision = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Demo_Collision Wall %d.csv", i);
		load_CSV(key->walls[i].collision, room_w, room_h, path);
	}

	key->floors = (Floor_Set*)calloc(n_floors, sizeof(Floor_Set));
	for (int i = 0; i < n_floors; ++i)
	{
		key->floors[i].floor = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Demo_Floor %d.csv", i);
		load_CSV(key->floors[i].floor, room_w, room_h, path);

		key->floors[i].collision = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Demo_Floor %d Collision.csv", i);
		load_CSV(key->floors[i].collision, room_w, room_h, path);

		key->floors[i].props = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Demo_Props %d.csv", i);
		load_CSV(key->floors[i].props, room_w, room_h, path);

		key->floors[i].spawns = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Demo_Spawn %d.csv", i);
		load_CSV(key->floors[i].spawns, room_w, room_h, path);
	}
}

struct Map
{
	int map_w, map_h, room_w, room_h;
	int** wall_collisions;
	int** floor_collisions;
	int** walls;
	int** floors;
	int** props;
	int** spawns;
	int* visited;
	Sheet* src_sheet;
};

int generate_Map(Map* map, int map_w, int map_h, const Map_Key key)
{
	int map_area = map_w * map_h;
	int room_area = map->room_w * map->room_h;

	map->map_w = map_w;
	map->map_h = map_h;

	//clear old key and allocate memory
	if (map->wall_collisions != NULL) free(map->wall_collisions);
	map->wall_collisions = (int**)calloc(map_area, sizeof(int*));
	if (map->floor_collisions != NULL) free(map->floor_collisions);
	map->floor_collisions = (int**)calloc(map_area, sizeof(int*));
	if (map->walls != NULL) free(map->walls);
	map->walls = (int**)calloc(map_area, sizeof(int*));
	if (map->floors != NULL) free(map->floors);
	map->floors = (int**)calloc(map_area, sizeof(int*));
	if (map->props != NULL) free(map->props);
	map->props = (int**)calloc(map_area, sizeof(int*));
	if (map->spawns != NULL) free(map->spawns);
	map->spawns = (int**)calloc(map_area, sizeof(int*));
	if (map->visited != NULL) free(map->visited);
	map->visited = (int*)calloc(map_area, sizeof(int));

	//start at the bottom, end at the top
	int maze_start = map_w * map_h - (rand() % map_w) - 1;
	int maze_end = rand() % map_w;

	int* maze = (int*)calloc(map_area, sizeof(int));
	int* track_stack = (int*)calloc(map_area, sizeof(int));
	int n_stack = 1;
	track_stack[0] = maze_start;
	int current_pos = maze_start;
	//depth-first maze gen
	while (n_stack > 0)
	{
		current_pos = track_stack[n_stack - 1];

		if (current_pos == maze_end || (
			((current_pos % map_w) - 1 < 0 || maze[current_pos - 1] != 0) &&
			((current_pos % map_w) + 1 >= map_w || maze[current_pos + 1] != 0) &&
			(current_pos - map_w < 0 || maze[current_pos - map_w] != 0) &&
			(current_pos + map_w >= map_area || maze[current_pos + map_w] != 0)))
			--n_stack;
		else
		{
			int direction = 1 + (rand() % 4);
			char dir_found = 0;
			while (dir_found == 0)
			{
				switch (direction)
				{
				case LEFT:
					if ((current_pos % map_w) - 1 < 0 || maze[current_pos - 1] != 0) ++direction;
					else
					{
						dir_found = 1;
						++maze[current_pos];
						--current_pos;
						track_stack[n_stack] = current_pos;
						++n_stack;
						maze[current_pos] += 2;
						break;
					}
				case RIGHT:
					if ((current_pos % map_w) + 1 >= map_w || maze[current_pos + 1] != 0) ++direction;
					else
					{
						dir_found = 1;
						maze[current_pos] += 2;
						++current_pos;
						track_stack[n_stack] = current_pos;
						++n_stack;
						++maze[current_pos];
						break;
					}
				case DOWN:
					if (current_pos + map_w >= map_area || maze[current_pos + map_w] != 0) ++direction;
					else
					{
						dir_found = 1;
						maze[current_pos] += 4;
						current_pos += map_w;
						track_stack[n_stack] = current_pos;
						++n_stack;
						maze[current_pos] += 8;
						break;
					}
				case UP:
					if (current_pos - map_w < 0 || maze[current_pos - map_w] != 0) direction = 1;
					else
					{
						dir_found = 1;
						maze[current_pos] += 8;
						current_pos -= map_w;
						track_stack[n_stack] = current_pos;
						++n_stack;
						maze[current_pos] += 4;
					}
					break;
				}
			}
		}
	}

	//save wall and floor sets to current map
	for (int i = 0; i < map_area; i++)
	{
		map->wall_collisions[i] = key.walls[maze[i]].collision;
		map->walls[i] = key.walls[maze[i]].visual;

		int floor_rand = rand() % key.n_floors;
		map->floor_collisions[i] = key.floors[floor_rand].collision;
		map->floors[i] = key.floors[floor_rand].floor;
		map->props[i] = key.floors[floor_rand].props;
		map->spawns[i] = key.floors[floor_rand].spawns;
	}

	//Set no hazards for starting room
	map->floors[maze_start] = key.floors[0].floor;
	map->floor_collisions[maze_start] = key.floors[0].collision;
	map->props[maze_start] = key.floors[0].props;
	map->spawns[maze_start] = key.floors[0].spawns;
	return maze_start;
}

//animation & entity states
enum {DEAD = 0, IDLE, WALK, ATTACK, DAMAGE, DYING};

//for drawing entity
enum {F_SIDE = 0, F_DOWN, F_UP};

struct Entity_Key
{
	Sheet* spritesheet;
	float speed;
	unsigned int max_hp;
	unsigned int idle_frame[3],
		walk_start[3], walk_end[3], walk_rate[3],
		atk_start[3], atk_end[3], atk_rate[3],
		dmg_start[3], dmg_end[3], dmg_rate[3],
		death_start[3], death_end[3], death_rate[3];
};

void load_Entity_Key(Entity_Key* entity, const char* eds_path, Sheet* sheet)
{
	entity->spritesheet = sheet;

	FILE* f_in = fopen(eds_path, "r");
	fscanf(f_in, "Idle Frame: %d-%d-%d\n", &entity->idle_frame[0], &entity->idle_frame[1], &entity->idle_frame[2]);
	fscanf(f_in, "Walk Start: %d-%d-%d\tWalk End: %d-%d-%d\tWalk Rate: %d-%d-%d\n", 
		&entity->walk_start[0], &entity->walk_start[1], &entity->walk_start[2],
		&entity->walk_end[0], &entity->walk_end[1], &entity->walk_end[2],
		&entity->walk_rate[0], &entity->walk_rate[1], &entity->walk_rate[2]);
	fscanf(f_in, "Attack Start: %d-%d-%d\tAttack End: %d-%d-%d\tAttack Rate: %d-%d-%d\n", 
		&entity->atk_start[0], &entity->atk_start[1], &entity->atk_start[2], 
		&entity->atk_end[0], &entity->atk_end[1], &entity->atk_end[2],
		&entity->atk_rate[0], &entity->atk_rate[1], &entity->atk_rate[2]);
	fscanf(f_in, "Damage Start: %d-%d-%d\tDamage End: %d-%d-%d\tDamage Rate: %d-%d-%d\n",
		&entity->dmg_start[0], &entity->dmg_start[1], &entity->dmg_start[2],
		&entity->dmg_end[0], &entity->dmg_end[1], &entity->dmg_end[2],
		&entity->dmg_rate[0], &entity->dmg_rate[1], &entity->dmg_rate[2]);
	fscanf(f_in, "Death Start: %d-%d-%d\tDeath End: %d-%d-%d\tDeath Rate: %d-%d-%d\n",
		&entity->death_start[0], &entity->death_start[1], &entity->death_start[2],
		&entity->death_end[0], &entity->death_end[1], &entity->death_end[2],
		&entity->death_rate[0], &entity->death_rate[1], &entity->death_rate[2]);
	/* ready for later once damage and death types are used
	fscanf(f_in, "Fall Start: %d-%d-%d\tFall End: %d-%d-%d\tFall Rate: %d-%d-%d\n",
		&entity->fall_start[0], &entity->fall_start[1], &entity->fall_start[2],
		&entity->fall_end[0], &entity->fall_end[1], &entity->fall_end[2],
		&entity->fall_rate[0], &entity->fall_rate[1], &entity->fall_rate[2]);
	*/
}

struct Entity_Instance
{
	Entity_Key* key;
	float x, y;
	unsigned int last_update, curr_frame;
	int curr_hp;
	char state, direction;
};

int check_Hit_Collision(Entity_Instance* def, const Entity_Instance atk, float atk_buffer)
{
	if (atk.direction == LEFT)
	{
		if (def->x < atk.x && def->x + 1.0f > atk.x - atk_buffer &&
			def->y < atk.y + 1.0f && def->y + 1.0f > atk.y)
		{
			--def->curr_hp;
			def->direction = RIGHT;
			if (def->curr_hp <= 0) def->state = DYING;
			else def->state = DAMAGE;
			return 1;
		}
	}
	else if (atk.direction == RIGHT)
	{
		if (def->x < atk.x + 1.0f + atk_buffer && def->x + 1.0f > atk.x + 1.0f &&
			def->y < atk.y + 1.0f && def->y + 1.0f > atk.y)
		{
			--def->curr_hp;
			def->direction = LEFT;
			if (def->curr_hp <= 0) def->state = DYING;
			else def->state = DAMAGE;
			return 1;
		}
	}
	else if (atk.direction == DOWN)
	{
		if (def->y < atk.y + atk_buffer && def->y + 1.0f > atk.y &&
			def->x < atk.x + 1.0f && def->x + 1.0f > atk.x)
		{
			--def->curr_hp;
			def->direction = UP;
			if (def->curr_hp <= 0) def->state = DYING;
			else def->state = DAMAGE;
			return 1;
		}
	}
	else
	{
		if (def->y < atk.y && def->y + 1.0f > atk.y - atk_buffer &&
			def->x < atk.x + 1.0f && def->x + 1.0f > atk.x)
		{
			--def->curr_hp;
			def->direction = DOWN;
			if (def->curr_hp <= 0) def->state = DYING;
			else def->state = DAMAGE;
			return 1;
		}
	}
	return 0;
}

void check_Tile_Collision(int* wall_c, int* floor_c, int room_w, Entity_Instance* e)
{
	int ix = (int)e->x;
	int iy = (int)e->y;
	float cx = e->x + 0.5f;
	float cy = e->y + 0.5f;

	if (wall_c[iy*room_w+ix] >= 0 || floor_c[iy*room_w + ix] >= 0)
	{
		
	}
}

//draw when room stationary
void draw_Entity_Instance(SDL_Renderer* renderer, const Entity_Instance e, int scale)
{
	SDL_Rect src = { (int)(e.curr_frame % e.key->spritesheet->n_cols) * scale, (int)(e.curr_frame / e.key->spritesheet->n_cols) * scale, scale, scale };
	SDL_Rect dest = { (int)(e.x * (float)scale), (int)(e.y * (float)scale), e.key->spritesheet->sprite_w, e.key->spritesheet->sprite_h };

	//flip to face correct direction
	if (e.direction == LEFT) SDL_RenderCopyEx(renderer, e.key->spritesheet->sheet, &src, &dest, 0, NULL, SDL_FLIP_NONE);
	else SDL_RenderCopyEx(renderer, e.key->spritesheet->sheet, &src, &dest, 0, NULL, SDL_FLIP_HORIZONTAL);
}

struct Camera
{
	float x, y, speed;
	int map_x, map_y, w, h, scale;
};

//draw during room swapping
void draw_Entity_Instance(SDL_Renderer* renderer, const Entity_Instance e, const Camera camera)
{
	int scale = camera.scale;
	SDL_Rect src = { (int)(e.curr_frame % e.key->spritesheet->n_cols) * scale, (int)(e.curr_frame / e.key->spritesheet->n_cols) * scale, scale, scale };
	SDL_Rect dest = { (int)((e.x - camera.x * (float)camera.w) * (float)scale), (int)((e.y - camera.y * (float)camera.h) * (float)scale), e.key->spritesheet->sprite_w, e.key->spritesheet->sprite_h };

	if (e.direction == LEFT) SDL_RenderCopyEx(renderer, e.key->spritesheet->sheet, &src, &dest, 0, NULL, SDL_FLIP_NONE);		//draw e
	else SDL_RenderCopyEx(renderer, e.key->spritesheet->sheet, &src, &dest, 0, NULL, SDL_FLIP_HORIZONTAL);
}

void update_Animation(Entity_Instance* e, unsigned int curr_time)
{
	if (e->state == DEAD) return;

	int direction = F_SIDE;
	if (e->direction == DOWN) direction = F_DOWN;
	else if (e->direction == UP) direction = F_UP;

	if (e->state == IDLE)
	{
		e->curr_frame = e->key->idle_frame[direction];
		return;
	}
	else if (e->state == WALK)
	{
		if (1000.0 / (curr_time - e->last_update) <= e->key->walk_rate[direction] || e->key->walk_rate[direction] <= 0)
		{
			e->last_update = curr_time;
			if (e->curr_frame < e->key->walk_start[direction] || e->curr_frame >= e->key->walk_end[direction]) e->curr_frame = e->key->walk_start[direction];
			else ++e->curr_frame;
		}
		return;
	}
	else if (e->state == ATTACK)
	{
		if (1000.0 / (curr_time - e->last_update) <= e->key->atk_rate[direction] || e->key->atk_rate[direction] <= 0)
		{
			e->last_update = curr_time;
			if (e->curr_frame < e->key->atk_start[direction] || e->curr_frame > e->key->atk_end[direction]) e->curr_frame = e->key->atk_start[direction];
			else if (e->curr_frame == e->key->atk_end[direction])
			{
				e->state = IDLE;
				e->curr_frame = e->key->idle_frame[direction];
			}
			else ++e->curr_frame;
		}
		return;
	}
	else if (e->state == DAMAGE)
	{
		if (e->direction == LEFT) e->x += e->key->speed;
		else if (e->direction == RIGHT) e->x -= e->key->speed;
		else if (e->direction == DOWN) e->y -= e->key->speed;
		else if (e->direction == UP) e->y += e->key->speed;

		if (1000.0 / (curr_time - e->last_update) <= e->key->dmg_rate[direction] || e->key->dmg_rate[direction] <= 0)
		{
			e->last_update = curr_time;
			if (e->curr_frame < e->key->dmg_start[direction] || e->curr_frame > e->key->dmg_end[direction]) e->curr_frame = e->key->dmg_start[direction];
			else if (e->curr_frame == e->key->dmg_end[direction])
			{
				e->state = IDLE;
				e->curr_frame = e->key->idle_frame[direction];
			}
			else ++e->curr_frame;
		}
		return;
	}
	else if (e->state == DYING)
	{
		if (1000.0 / (curr_time - e->last_update) <= e->key->death_rate[direction] || e->key->death_rate[direction] <= 0)
		{
			e->last_update = curr_time;
			if (e->curr_frame < e->key->death_start[direction] || e->curr_frame > e->key->death_end[direction]) e->curr_frame = e->key->death_start[direction];
			else if (e->curr_frame == e->key->death_end[direction]) e->state = DEAD;
			else ++e->curr_frame;
		}
		return;
	}
}

//drawing non-moving room
void draw_Room_Layer(SDL_Renderer* renderer, const int* layer, const Sheet* src_sheet, int _room_w, int _room_h, int sprite_scale)
{
	int room_w = _room_w;
	int room_h = _room_h;
	int tile_size = sprite_scale;

	SDL_Rect src = { 0 };
	src.w = src_sheet->sprite_w;
	src.h = src_sheet->sprite_h;

	SDL_Rect dest = { 0 };
	dest.w = src_sheet->sprite_w;
	dest.h = src_sheet->sprite_w;

	const int* data = layer;
	for (int y = 0; y < room_h; y++)
	{
		for (int x = 0; x < room_w; x++)
		{
			src.x = (data[y * room_w + x] % src_sheet->n_cols) * src.w;
			src.y = (data[y * room_w + x] / src_sheet->n_cols) * src.h;

			SDL_RenderCopy(renderer, src_sheet->sheet, &src, &dest);

			dest.x += dest.w;
		}
		dest.x = 0;
		dest.y += dest.h;
	}
}

//drawing moving rooms
void draw_Rooms_Layers(SDL_Renderer* renderer, int** layers, int layer_1_id, int direction, const Sheet* src_sheet, int _room_w, int _room_h, int _map_w, Camera* camera)
{
	int room_w = _room_w;
	int room_h = _room_h;
	int map_w = _map_w;
	int tile_size = camera->scale;

	SDL_Rect src = { 0 };
	src.w = src_sheet->sprite_w;
	src.h = src_sheet->sprite_h;

	int start_x = (int)(0.0f - camera->x * (float)room_w * camera->scale);
	int start_y = (int)(0.0f - camera->y * (float)room_h * camera->scale);
	SDL_Rect dest = { 0 };
	dest.x = start_x;
	dest.y = start_y;
	dest.w = src_sheet->sprite_w;
	dest.h = src_sheet->sprite_h;

	const int* data = layers[layer_1_id];

	for (int y = 0; y < room_h; y++)
	{
		if (dest.y > camera->scale * -1)
		{
			for (int x = 0; x < room_w; x++)
			{
				if (dest.x > camera->scale * -1)
				{
					src.x = (data[y * room_w + x] % src_sheet->n_cols) * src.w;
					src.y = (data[y * room_w + x] / src_sheet->n_cols) * src.h;

					SDL_RenderCopy(renderer, src_sheet->sheet, &src, &dest);
				}
				dest.x += dest.w;
			}
		}
		dest.x = start_x;
		dest.y += dest.h;
	}

	int layer_2_id = layer_1_id;
	if (direction == UP)
	{
		layer_2_id -= map_w;
		start_y -= room_h * tile_size;
	}
	else if (direction == DOWN)
	{
		layer_2_id += map_w;
		start_y += room_h * tile_size;
	}
	else if (direction == LEFT)
	{
		--layer_2_id;
		start_x -= room_w * tile_size;
	}
	else
	{
		++layer_2_id;
		start_x += room_w * tile_size;
	}
	dest.x = start_x;
	dest.y = start_y;
	data = layers[layer_2_id];

	for (int y = 0; y < room_h; y++)
	{
		if (dest.y > camera->scale * -1)
		{
			for (int x = 0; x < room_w; x++)
			{
				if (dest.x > camera->scale * -1)
				{
					src.x = (data[y * room_w + x] % src_sheet->n_cols) * src.w;
					src.y = (data[y * room_w + x] / src_sheet->n_cols) * src.h;

					SDL_RenderCopy(renderer, src_sheet->sheet, &src, &dest);
				}
				dest.x += dest.w;
			}
		}
		dest.x = start_x;
		dest.y += dest.h;
	}
}

void draw_Collision_Layer(SDL_Renderer* renderer, SDL_Texture* blue, const int* layer, int room_w, int room_h, int box_w, int box_h, int scale)
{
	SDL_Rect src = { 0 };
	src.w = box_w;
	src.h = box_h;

	SDL_Rect dest = { 0 };
	dest.w = scale;
	dest.h = scale;

	const int* data = layer;
	for (int y = 0; y < room_h; y++)
	{
		for (int x = 0; x < room_w; x++)
		{
			if (layer[y*room_w + x] >= 0) SDL_RenderCopy(renderer, blue, &src, &dest);
			dest.x += dest.w;
		}
		dest.x = 0;
		dest.y += dest.h;
	}
}

void draw_Entity_Collision(SDL_Renderer* renderer, SDL_Texture* green, SDL_Texture* red, int box_w, int box_h, const Entity_Instance e, int scale, float atk_range)
{
	SDL_Rect src = { 0, 0, box_w, box_h };
	SDL_Rect dest = { (int)(e.x * (float)scale), (int)(e.y * (float)scale), e.key->spritesheet->sprite_w, e.key->spritesheet->sprite_h };

	SDL_RenderCopy(renderer, green, &src, &dest);

	if (e.state == ATTACK)
	{
		if (e.direction == LEFT)
		{
			dest.w *= atk_range;
			dest.x -= dest.w;
		}
		else if (e.direction == RIGHT)
		{
			dest.x += dest.w;
			dest.w *= atk_range;
		}
		else if (e.direction == DOWN)
		{
			dest.y += dest.h;
			dest.h *= atk_range;
		}
		else if (e.direction == UP)
		{
			dest.h *= atk_range;
			dest.y -= dest.h;
		}
		SDL_RenderCopy(renderer, red, &src, &dest);
	}
}

int main(int argc, char** argv)
{
	srand((unsigned int)time(0));
	//SDL Init
	SDL_Init(SDL_INIT_EVERYTHING);
	unsigned short window_w = 1024;
	unsigned short window_h = 768;
	SDL_Window* window = SDL_CreateWindow("Hero Chicken", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	//System Init
	unsigned int curr_time = 0;
	unsigned int last_update_time = 0;
	short max_framerate = 60;
	char w_state = 0;
	char a_state = 0;
	char s_state = 0;
	char d_state = 0;
	char t_state = 0;
	char space_state = 0;
	char prev_w_state = 0;
	char prev_a_state = 0;
	char prev_s_state = 0;
	char prev_d_state = 0;
	char prev_t_state = 0;
	char prev_space_state = 0;
	float atk_range = 0.25;

	//font init
	Text::Font font = { 0 };
	font.src = NULL;
	load_Image_And_Size_To_Texture(&font.src, &font.src_w, &font.src_h, renderer, "Assets/Images/fontsheet.png");
	font.font_w = 64;
	font.font_h = 96;
	font.n_cols = font.src_w / font.font_w;

	//camera for room movement
	Camera camera = { 0 };
	camera.w = 16;
	camera.h = 12;
	camera.scale = 64;
	camera.speed = 0.015f;
	int direction = MID;
	int prev_direction = MID;
	
	//to show collisions
	char debug_toggle = -1;
	int col_box_w = 0;
	int col_box_h = 0;
	SDL_Texture* red = { 0 };
	load_Image_And_Size_To_Texture(&red, &col_box_w, &col_box_h, renderer, "Assets/Images/Red Overlay.png");
	SDL_Texture* green = { 0 };
	load_Image_To_Texture(&green, renderer, "Assets/Images/Green Overlay.png");
	SDL_Texture* blue = { 0 };
	load_Image_To_Texture(&blue, renderer, "Assets/Images/Blue Overlay.png");

	//Player Init
	Sheet chicken_sheet = { 0 };
	load_Sheet(&chicken_sheet, 64, 64, renderer, "Assets/Images/PlayerSpritesheet.png");

	Entity_Key chicken_key = { 0 };
	load_Entity_Key(&chicken_key, "Assets/EDS/Chicken.eds", &chicken_sheet);
	chicken_key.max_hp = 3;

	Entity_Instance p1 = { 0 };
	p1.key = &chicken_key;
	p1.x = 7.5f;
	p1.y = 5.5f;
	p1.key->speed = 0.1f;
	p1.direction = DOWN;
	p1.state = IDLE;
	p1.curr_hp = p1.key->max_hp;

	int player_money = -20;

	Sheet enemy_sheet = { 0 };
	load_Sheet(&enemy_sheet, 64, 64, renderer, "Assets/Images/EnemySpritesheet.png");

	Entity_Key enemy_key = { 0 };
	load_Entity_Key(&enemy_key, "Assets/EDS/Enemy.eds", &enemy_sheet);
	enemy_key.speed = 0.05f;
	enemy_key.max_hp = 1;

	const int max_enemies = 5;
	int n_enemies = 0;
	Entity_Instance enemies[max_enemies] = { 0 };
	for (int i = 0; i < max_enemies; ++i) enemies[i].key = &enemy_key;

	Map_Key key = { 0 };
	load_Map_Key(&key, 16, 12, 16, 11);

	//load Locale
	Sheet terrain_sheet = { 0 };
	load_Sheet(&terrain_sheet, camera.scale, camera.scale, renderer, "Assets/Images/Modular_Tileset_Dungeon.png");

	Sheet prop_sheet = { 0 };
	load_Sheet(&prop_sheet, camera.scale, camera.scale, renderer, "Assets/Images/Propsheet.png");

	Map dungeon_map = { 0 };
	dungeon_map.src_sheet = &terrain_sheet;
	dungeon_map.room_w = 16;
	dungeon_map.room_h = 12;

	static int locale[5] = { 0 };

	locale[MID] = generate_Map(&dungeon_map, 3, 3, key);
	locale[LEFT] = locale[MID] - 1;
	locale[RIGHT] = locale[MID] + 1;
	locale[DOWN] = locale[MID] + dungeon_map.map_w;
	locale[UP] = locale[MID] - dungeon_map.map_w;
	++dungeon_map.visited[locale[MID]];

	float lava_roam = 0.0f;
	float lava_speed = 0.25f;

	for (;;)
	{
		curr_time = SDL_GetTicks();
		if (1000.0 / (curr_time - last_update_time) <= max_framerate || max_framerate <= 0)
		{
			//update system
			last_update_time = curr_time;
			prev_direction = direction;
			prev_w_state = w_state;
			prev_a_state = a_state;
			prev_s_state = s_state;
			prev_d_state = d_state;
			prev_t_state = t_state;
			prev_space_state = space_state;
			SDL_Event event;
			while (SDL_PollEvent(&event) == 1)
			{
				if (event.type == SDL_QUIT) exit(0);
				else if (event.type == SDL_KEYDOWN)
				{
					char key = event.key.keysym.sym;
					if (key == SDLK_ESCAPE) exit(0);
					else if (key == SDLK_w) w_state = 1;
					else if (key == SDLK_a) a_state = 1;
					else if (key == SDLK_s) s_state = 1;
					else if (key == SDLK_d) d_state = 1;
					else if (key == SDLK_t) t_state = 1;
					else if (key == SDLK_SPACE) space_state = 1;
				}
				else if (event.type == SDL_KEYUP)
				{
					char key = event.key.keysym.sym;
					if (key == SDLK_w) w_state = 0;
					else if (key == SDLK_a) a_state = 0;
					else if (key == SDLK_s) s_state = 0;
					else if (key == SDLK_d) d_state = 0;
					else if (key == SDLK_t) t_state = 0;
					else if (key == SDLK_SPACE) space_state = 0;
				}
			}

			if (t_state == 1 && prev_t_state == 0) debug_toggle *= -1;

			//input physics
			if (direction == MID)
			{
				if (space_state == 1 && prev_space_state == 0 && (p1.state == IDLE || p1.state == WALK))
				{
					p1.state = ATTACK;
				}
				else
				{
					if (p1.state != ATTACK) p1.state = IDLE;
					if (w_state == 1)
					{
						p1.state = WALK;
						p1.direction = UP;
						p1.y -= p1.key->speed;
					}
					if (a_state == 1)
					{
						p1.state = WALK;
						p1.direction = LEFT;
						p1.x -= p1.key->speed;
					}
					if (s_state == 1)
					{
						p1.state = WALK;
						p1.direction = DOWN;
						p1.y += p1.key->speed;
					}
					if (d_state == 1)
					{
						p1.state = WALK;
						p1.direction = RIGHT;
						p1.x += p1.key->speed;
					}
				}
				if (n_enemies <= 0)
				{
					if (p1.y < 0.0 && locale[UP] >= 0)
					{
						direction = UP;
						--camera.map_y;
					}
					else if (p1.x < 0.0 && locale[LEFT] % dungeon_map.map_w >= 0)
					{
						direction = LEFT;
						--camera.map_x;
					}
					else if (p1.y > dungeon_map.room_h - 1 && locale[DOWN] / dungeon_map.map_w <= dungeon_map.map_h)
					{
						direction = DOWN;
						++camera.map_y;
					}
					else if (p1.x > dungeon_map.room_w - 1 && locale[RIGHT] % dungeon_map.map_w <= dungeon_map.map_w)
					{
						direction = RIGHT;
						++camera.map_x;
					}
				}
				
				//combat
				if (p1.state == ATTACK)
				{
					int n_hits = 0;
					for (int i = 0; i < max_enemies; ++i)
					{
						if (enemies[i].state != DEAD && enemies[i].state != DAMAGE && enemies[i].state != DYING)
						{
							n_hits += check_Hit_Collision(&enemies[i], p1, atk_range);
						}
					}
					player_money += n_hits * 10;
					n_enemies -= n_hits;
				}
				else if (p1.state != DEAD && p1.state != DAMAGE && p1.state != DYING)
				{
					for (int i = 0; i < max_enemies; ++i)
					{
						if (enemies[i].state == ATTACK)
						{
							check_Hit_Collision(&p1, enemies[i], atk_range);
						}
					}
				}
				//check collision

			}
			else
			{
				p1.state = IDLE;
				if (direction == UP)
				{
					if (p1.y > -3.0f)
					{
						p1.state = WALK;
						p1.direction = UP;
						p1.y = p1.y - p1.key->speed;
					}
					camera.y = camera.y - camera.speed;
					if (camera.y <= -1.0f)
					{
						camera.y = 0.0f;
						for (int i = 0; i < 5; i++) locale[i] -= dungeon_map.map_w;
						p1.y = (float)dungeon_map.room_h - 2.0f;
						direction = MID;
					}
				}
				else if (direction == DOWN)
				{
					if (p1.y < dungeon_map.room_h + 2.0f)
					{
						p1.state = WALK;
						p1.direction = DOWN;
						p1.y = p1.y + p1.key->speed;
					}
					camera.y = camera.y + camera.speed;
					if (camera.y >= 1.0f)
					{
						camera.y = 0.0;
						for (int i = 0; i < 5; i++) locale[i] += dungeon_map.map_w;
						p1.y = 1.0;
						direction = MID;
					}
				}
				else if (direction == LEFT)
				{
					if (p1.x > -3.0f)
					{
						p1.state = WALK;
						p1.direction = LEFT;
						p1.x = p1.x - p1.key->speed;
					}
					camera.x = camera.x - camera.speed;
					if (camera.x <= -1.0f)
					{
						camera.x = 0.0f;
						for (int i = 0; i < 5; i++) locale[i] -= 1;
						p1.x = dungeon_map.room_w - 2.0f;
						direction = MID;
					}
				}
				else if (direction == RIGHT)
				{
					if (p1.x < dungeon_map.room_w + 2.0f)
					{
						p1.state = WALK;
						p1.direction = RIGHT;
						p1.x = p1.x + p1.key->speed;
					}
					camera.x = camera.x + camera.speed;
					if (camera.x >= 1.0f)
					{
						camera.x = 0.0f;
						for (int i = 0; i < 5; i++) locale[i] += 1;
						p1.x = 1.0f;
						direction = MID;
					}
				}

				if (prev_direction != direction && dungeon_map.visited[locale[MID]] <= 0)
				{
					++dungeon_map.visited[locale[MID]];
					for (int i = 0; i < dungeon_map.room_w * dungeon_map.room_h; ++i)
					{
						if (n_enemies >= max_enemies) break;
						if (dungeon_map.spawns[locale[MID]][i] >= 0)
						{
							enemies[n_enemies].state = IDLE;
							enemies[n_enemies].direction = DOWN;
							enemies[n_enemies].x = (float)(i % dungeon_map.room_w);
							enemies[n_enemies].y = (float)(i / dungeon_map.room_w);
							enemies[n_enemies].curr_hp = enemies[n_enemies].key->max_hp;
							++n_enemies;
						}
					}
				}
			}

			//update animations
			for (int i = 0; i < max_enemies; ++i) if (enemies[i].state > 0) update_Animation(&enemies[i], curr_time);
			update_Animation(&p1, curr_time);

			//draw
			SDL_RenderClear(renderer);
			lava_roam = lava_roam + lava_speed;
			if (lava_roam >= 64.0f) lava_roam = lava_roam - 64.0f;
			SDL_Rect backdrop_src = { 5 * 64, 64, 64, 64 };
			SDL_Rect backdrop_dest = { 0 - (int)lava_roam, 0 - (int)lava_roam, 64, 64 };
			for (int y = -1; y < dungeon_map.room_h; ++y)
			{
				for (int x = -1; x < dungeon_map.room_w; ++x)
				{
					SDL_RenderCopy(renderer, terrain_sheet.sheet, &backdrop_src, &backdrop_dest);
					backdrop_dest.x += 64;
				}
				backdrop_dest.x = 0 - (int)lava_roam;
				backdrop_dest.y += 64;
			}
			if (direction == MID)
			{
				draw_Room_Layer(renderer, dungeon_map.floors[locale[MID]],
					&terrain_sheet, dungeon_map.room_w, dungeon_map.room_h, camera.scale);
				draw_Room_Layer(renderer, dungeon_map.props[locale[MID]],
					&prop_sheet, dungeon_map.room_w, dungeon_map.room_h, camera.scale);
				for (int i = 0; i < max_enemies; ++i)
				{
					if (enemies[i].state != DEAD) draw_Entity_Instance(renderer, enemies[i], camera.scale);
				}
				draw_Entity_Instance(renderer, p1, camera.scale);
				draw_Room_Layer(renderer, dungeon_map.walls[locale[MID]],
					&terrain_sheet, dungeon_map.room_w, dungeon_map.room_h, camera.scale);

				//draw collisions for debug
				if (debug_toggle == 1)
				{
					draw_Collision_Layer(renderer, blue, dungeon_map.floor_collisions[locale[MID]], dungeon_map.room_w, dungeon_map.room_h, col_box_w, col_box_h, camera.scale);
					draw_Collision_Layer(renderer, blue, dungeon_map.wall_collisions[locale[MID]], dungeon_map.room_w, dungeon_map.room_h, col_box_w, col_box_h, camera.scale);
					draw_Entity_Collision(renderer, green, red, col_box_w, col_box_h, p1, camera.scale, atk_range);
					for (int i = 0; i < max_enemies; ++i)
					{
						if (enemies[i].state != DEAD) draw_Entity_Collision(renderer, green, red, col_box_w, col_box_h, enemies[i], camera.scale, atk_range);
					}
				}
			}
			else
			{
				draw_Rooms_Layers(renderer, dungeon_map.floors, locale[MID], direction,
					&terrain_sheet, dungeon_map.room_w, dungeon_map.room_h, dungeon_map.map_w, &camera);
				draw_Rooms_Layers(renderer, dungeon_map.props, locale[MID], direction,
					&prop_sheet, dungeon_map.room_w, dungeon_map.room_h, dungeon_map.map_w, &camera);
				draw_Entity_Instance(renderer, p1, camera);
				draw_Rooms_Layers(renderer, dungeon_map.walls, locale[MID], direction,
					&terrain_sheet, dungeon_map.room_w, dungeon_map.room_h, dungeon_map.map_w, &camera);
			}
			//display health

			//display money
			Text::draw_Text(renderer, "Money: ", 0, 0, 250, &font, 21);
			Text::draw_Int(renderer, player_money, 140, 0, 250, &font, 21);
			SDL_RenderPresent(renderer);
		}
	}
}