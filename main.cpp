#pragma comment (linker, "/subsystem:console")

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
		sprintf(path, "Assets/CSV/Cave_Walls_%d_Visual.csv", i);
		load_CSV(key->walls[i].visual, room_w, room_h, path);

		key->walls[i].collision = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Walls_%d_Collision.csv", i);
		load_CSV(key->walls[i].collision, room_w, room_h, path);
	}

	key->floors = (Floor_Set*)calloc(n_floors, sizeof(Floor_Set));
	for (int i = 0; i < n_floors; ++i)
	{
		key->floors[i].floor = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Floors_%d_Visual.csv", i);
		load_CSV(key->floors[i].floor, room_w, room_h, path);

		key->floors[i].collision = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Floors_%d_Collision.csv", i);
		load_CSV(key->floors[i].collision, room_w, room_h, path);

		key->floors[i].props = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Floors_%d_Props.csv", i);
		load_CSV(key->floors[i].props, room_w, room_h, path);

		key->floors[i].spawns = (int*)calloc(room_area, sizeof(int));
		sprintf(path, "Assets/CSV/Cave_Floors_%d_Spawn.csv", i);
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

struct Node
{
	int x, y, px, py, h_cost;
};

struct Heap
{
	int n_data, max_data;
	int* id;
};

void add_To_Heap(Heap* heap, Node* data, int input, int room_w)
{
	heap->id[heap->n_data] = input;
	int pos = heap->n_data;
	int parent_pos = (heap->n_data - 1) / 2;
	++heap->n_data;
	//printf("Adding %d\n", input);
	while (pos > 0 && parent_pos >= 0 && data[heap->id[pos]].h_cost < data[heap->id[parent_pos]].h_cost)
	{
		int temp = heap->id[parent_pos];
		heap->id[parent_pos] = heap->id[pos];
		heap->id[pos] = temp;
		pos = parent_pos;
		parent_pos = (pos - 1) / 2;
	}
	//for (int i = 0; i < heap->n_data; ++i) printf("%d ", data[heap->id[i]].h_cost);
	//printf("\n");
}

int get_From_Heap(Heap* heap, Node* data)
{
	int gotten_node = heap->id[0];
	--heap->n_data;
	//printf("Removing %d\n", gotten_node);
	heap->id[0] = heap->id[heap->n_data];
	if (heap->n_data <= 1) return gotten_node;

	int pos = 0;
	int child_pos = 1;
	if (child_pos < heap->n_data - 1 && data[heap->id[child_pos]].h_cost > data[heap->id[child_pos + 1]].h_cost) ++child_pos;

	while (data[heap->id[pos]].h_cost > data[heap->id[child_pos]].h_cost)
	{
		//printf("Swap!\n");
		int temp = heap->id[child_pos];
		heap->id[child_pos] = heap->id[pos];
		heap->id[pos] = temp;

		pos = child_pos;
		child_pos = (pos * 2) + 1;
		if (child_pos >= heap->n_data) break;
		if (child_pos < heap->n_data - 1 && data[heap->id[child_pos]].h_cost > data[heap->id[child_pos + 1]].h_cost) ++child_pos;
	}
	//for (int i = 0; i < heap->n_data; ++i) printf("%d ", data[heap->id[i]].h_cost);
	//printf("\n");
	return gotten_node;
}

void check_successor(Heap* heap, Node* data, int* closed, int n_closed, const Node successor, int* wall_col, int* floor_col, int room_w, int dx, int dy)
{
	int succ_id = successor.y * room_w + successor.x;
	if (wall_col[succ_id] < 0 && floor_col[succ_id] < 0)
	{
		int skip = 0;
		for (int i = 0; i < heap->n_data; ++i)
		{
			if (heap->id[i] == succ_id)
			{
				if (data[heap->id[i]].h_cost < successor.h_cost && data[heap->id[i]].h_cost != -1)
				{
					skip = 1;
				}
				i = heap->n_data;
			}
		}
		if (skip == 0)
		{
			for (int i = 0; i < n_closed; ++i)
			{
				if (closed[i] == succ_id)
				{
					if (data[closed[i]].h_cost <= successor.h_cost && data[closed[i]].h_cost != -1)
					{
						skip = 1;
					}
					i = n_closed;
				}
			}
			if (skip == 0)
			{
				data[succ_id].px = successor.px;
				data[succ_id].py = successor.py;
				add_To_Heap(heap, data, succ_id, room_w);
			}
		}
	}
}

void move_To_Position(Entity_Instance* e, int* wall_col, int* floor_col, int room_w, int room_h, float dest_x, float dest_y)
{
	int ex = (int)e->x;
	int ey = (int)e->y;
	//printf("True: %f %f\n", e->x, e->y);
	//printf("Assumed: %f %f\n", (float)ex, (float)ey);
	if ((float)ex != e->x)
	{
		if (fabs((float)ex - e->x) < e->key->speed - 0.01f)
		{
			//printf("Correct Left\n");
			e->x = (float)ex;
		}
		else if (fabs((float)ex - e->x) > 1.01f - e->key->speed)
		{
			//printf("Correct Right\n");
			e->x = (float)ex + 1.0f;
		}
		else
		{
			if (e->direction == LEFT) e->x -= e->key->speed;
			else e->x += e->key->speed;
			return;
		}
	}
	else if ((float)ey != e->y)
	{
		if (fabs((float)ey - e->y) < e->key->speed - 0.01f)
		{
			//printf("Correct Up\n");
			e->y = (float)ey;
		}
		else if (fabs((float)ey - e->y) > 1.01f - e->key->speed)
		{
			//printf("Correct Down\n");
			e->y = (float)ey + 1.0f;
		}
		else
		{
			if (e->direction == UP) e->y -= e->key->speed;
			else e->y += e->key->speed;
			return;
		}
	}
	int dx = (int)(dest_x + 0.5);
	int dy = (int)(dest_y + 0.5);
	int direction = 0;

	//a* to get direction
	Node* data = (Node*)calloc(room_w * room_h, sizeof(Node));
	for (int i = 0; i < room_w * room_h; ++i)
	{
		data[i].x = i % room_w;
		data[i].y = i / room_w;
		data[i].h_cost = abs(data[i].x - dx) + abs(data[i].y - dy);
	}

	Heap open = { 0 };
	open.max_data = room_w * room_h;
	open.id = (int*)calloc(open.max_data, sizeof(int));

	int* closed = (int*)calloc(open.max_data, sizeof(int));
	int n_closed = 0;

	int curr_id = ey * room_w + ex;
	Node curr_node = { 0 };
	curr_node.x = ex;
	curr_node.y = ey;
	curr_node.h_cost = abs(ex - dx) + abs(ey - dy);
	add_To_Heap(&open, data, curr_id, room_w);

	Node successor = { 0 };

	int search = 1;

	//find path
	while (open.n_data > 0 && search == 1)
	{
		curr_id = get_From_Heap(&open, data);
		curr_node = data[curr_id];
		//printf("Current Position: %d %d %d\n", curr_node.x, curr_node.y, curr_node.h_cost);
		successor = curr_node;
		successor.px = curr_node.x;
		successor.py = curr_node.y;
		if (curr_node.x > 0)
		{
			--successor.x;
			if (wall_col[successor.y * room_w + successor.x] < 0 && floor_col[successor.y * room_w + successor.x] < 0)
			{
				successor.h_cost = abs(successor.x - dx) + abs(successor.y - dy);
				//printf("Success Position: %d %d %d\n", curr_node.x, curr_node.y, curr_node.h_cost);
				if (successor.h_cost == 0)
				{
					data[successor.y * room_w + successor.x].px = successor.px;
					data[successor.y * room_w + successor.x].py = successor.py;
					search = 0;
					break;
				}
				else check_successor(&open, data, closed, n_closed, successor, wall_col, floor_col, room_w, dx, dy);
			}
			++successor.x;
		}
		if (curr_node.x < room_w - 1)
		{
			++successor.x;
			if (wall_col[successor.y * room_w + successor.x] < 0 && floor_col[successor.y * room_w + successor.x] < 0)
			{
				successor.h_cost = abs(successor.x - dx) + abs(successor.y - dy);
				//printf("Success Position: %d %d %d\n", curr_node.x, curr_node.y, curr_node.h_cost);
				if (successor.h_cost == 0)
				{
					data[successor.y * room_w + successor.x].px = successor.px;
					data[successor.y * room_w + successor.x].py = successor.py;
					search = 0;
					break;
				}
				else check_successor(&open, data, closed, n_closed, successor, wall_col, floor_col, room_w, dx, dy);
			}
			--successor.x;
		}
		if (curr_node.y > 0)
		{
			--successor.y;
			if (wall_col[successor.y * room_w + successor.x] < 0 && floor_col[successor.y * room_w + successor.x] < 0)
			{
				successor.h_cost = abs(successor.x - dx) + abs(successor.y - dy);
				//printf("Success Position: %d %d %d\n", curr_node.x, curr_node.y, curr_node.h_cost);
				if (successor.h_cost == 0)
				{
					data[successor.y * room_w + successor.x].px = successor.px;
					data[successor.y * room_w + successor.x].py = successor.py;
					search = 0;
					break;
				}
				else check_successor(&open, data, closed, n_closed, successor, wall_col, floor_col, room_w, dx, dy);
			}
			++successor.y;
		}
		if (curr_node.y < room_h - 1)
		{
			++successor.y;
			if (wall_col[successor.y * room_w + successor.x] < 0 && floor_col[successor.y * room_w + successor.x] < 0)
			{
				successor.h_cost = abs(successor.x - dx) + abs(successor.y - dy);
				//printf("Success Position: %d %d %d\n", curr_node.x, curr_node.y, curr_node.h_cost);
				if (successor.h_cost == 0)
				{
					data[successor.y * room_w + successor.x].px = successor.px;
					data[successor.y * room_w + successor.x].py = successor.py;
					search = 0;
					break;
				}
				else check_successor(&open, data, closed, n_closed, successor, wall_col, floor_col, room_w, dx, dy);
			}
			--successor.y;
		}
		closed[n_closed] = curr_id;
		++n_closed;
	}

	//get direction
	//printf("End found\n");
	curr_id = dy * room_w + dx;
	int start_id = ey * room_w + ex;
	int n_steps = 0;
	while (curr_id != start_id)
	{
		++n_steps;
		if (data[curr_id].px == 0 || data[curr_id].py == 0) break;

		if (data[curr_id].px - data[curr_id].x < 0) direction = RIGHT;
		else if (data[curr_id].px - data[curr_id].x > 0) direction = LEFT;
		else if (data[curr_id].py - data[curr_id].y > 0) direction = UP;
		else direction = DOWN;
		//printf("%d %d %d\n", data[curr_id].x, data[curr_id].y, direction);
		curr_id = data[curr_id].py * room_w + data[curr_id].px;
	}

	//deallocate
	free(open.id);
	free(data);
	free(closed);

	//printf("Before: %f %f\n", e->x, e->y);
	//printf("Assumed: %d %d\n", ex, ey);
	//move once direction known
	e->state = WALK;
	e->direction = direction;
	if (n_steps <= 1) e->state = ATTACK;
	else if (direction == LEFT)
	{
		if ((float)ey != e->y && (wall_col[(ey+1) * room_w + ex - 1] >= 0 || floor_col[(ey+1) * room_w + ex - 1] >= 0))
		{
			e->direction = UP;
			e->y -= e->key->speed;
		}
		else e->x -= e->key->speed;
	}
	else if (direction == RIGHT)
	{
		if ((float)ey != e->y && (wall_col[(ey + 1) * room_w + ex + 1] >= 0 || floor_col[(ey + 1) * room_w + ex + 1] >= 0))
		{
			e->direction = UP;
			e->y -= e->key->speed;
		}
		else e->x += e->key->speed;
	}
	else if (direction == DOWN) e->y += e->key->speed;
	else if (direction == UP) e->y -= e->key->speed;

	//printf("After: %f %f\n", e->x, e->y);
}

int check_Hit_Collision(Entity_Instance* def, const Entity_Instance atk, float atk_buffer)
{
	if (atk.direction == LEFT)
	{
		if (def->x < atk.x && def->x + 1.0f > atk.x - atk_buffer &&
			def->y < atk.y + 1.0f && def->y + 1.0f > atk.y)
		{
			--def->curr_hp;
			def->direction = RIGHT;
			if (def->curr_hp <= 0)
			{
				def->state = DYING;
				return 2;
			}
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
			if (def->curr_hp <= 0)
			{
				def->state = DYING;
				return 2;
			}
			else def->state = DAMAGE;
			return 1;
		}
	}
	else if (atk.direction == DOWN)
	{
		if (def->y < atk.y + 1.0f + atk_buffer && def->y + 1.0f > atk.y + 1.0f &&
			def->x < atk.x + 1.0f && def->x + 1.0f > atk.x)
		{
			--def->curr_hp;
			def->direction = UP;
			if (def->curr_hp <= 0)
			{
				def->state = DYING;
				return 2;
			}
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
			if (def->curr_hp <= 0)
			{
				def->state = DYING;
				return 2;
			}
			else def->state = DAMAGE;
			return 1;
		}
	}
	return 0;
}

void check_Tile_Collision(int* wall_c, int* floor_c, int room_w, Entity_Instance* e, int n_enemies)
{
	int ix = (int)e->x;
	int iy = (int)e->y;

	if ((wall_c[iy*room_w + ix] == 1 || (n_enemies > 0 && wall_c[iy*room_w+ix] >= 0)) || floor_c[iy*room_w + ix] >= 0)
	{
		float dif1 = e->x - (float)ix;
		float dif2 = e->y - (float)iy;
		if (dif1 > dif2) e->x = (float)(ix + 1);
		else e->y = (float)(iy + 1);
		ix = (int)e->x;
		iy = (int)e->y;
	}
	if ((float)ix != e->x)
	{
		if ((wall_c[iy*room_w + ix + 1] == 1 || (n_enemies > 0 && wall_c[iy*room_w + ix + 1] >= 0)) || floor_c[iy*room_w + ix + 1] >= 0)
		{
			float dif1 = (float)(ix + 1) - e->x;
			float dif2 = e->y - (float)iy;
			if (dif1 > dif2) e->x = (float)ix;
			else e->y = (float)(iy + 1);
			ix = (int)e->x;
			iy = (int)e->y;
		}
	}

	if ((wall_c[(iy + 1) * room_w + ix] == 1 || (n_enemies > 0 && wall_c[(iy + 1) * room_w + ix] >= 0)) || floor_c[(iy + 1) * room_w + ix] >= 0)
	{
		float dif1 = e->x - (float)ix;
		float dif2 = (float)(iy + 1) - e->y;
		if (dif1 > dif2) e->x = (float)(ix + 1);
		else e->y = (float)iy;
		ix = (int)e->x;
		iy = (int)e->y;
	}
	if ((float)ix != e->x)
	{
		if ((wall_c[(iy + 1) * room_w + ix + 1] == 1 || (n_enemies > 0 && wall_c[(iy + 1) * room_w + ix + 1] >= 0)) || floor_c[(iy + 1) * room_w + ix + 1] >= 0)
		{
			float dif1 = (float)(ix + 1) - e->x;
			float dif2 = (float)(iy + 1) - e->y;
			if (dif1 > dif2) e->x = (float)ix;
			else e->y = (float)iy;
			ix = (int)e->x;
			iy = (int)e->y;
		}
	}
}

struct Camera
{
	float x, y, speed;
	int map_x, map_y, w, h, scale;
};

//draw when room stationary
void draw_Entity_Instance(SDL_Renderer* renderer, const Entity_Instance e, int scale)
{
	SDL_Rect src = { (int)(e.curr_frame % e.key->spritesheet->n_cols) * scale, (int)(e.curr_frame / e.key->spritesheet->n_cols) * scale, scale, scale };
	SDL_Rect dest = { (int)(e.x * (float)scale), (int)(e.y * (float)scale), e.key->spritesheet->sprite_w, e.key->spritesheet->sprite_h };

	//flip to face correct direction
	if (e.direction == LEFT) SDL_RenderCopyEx(renderer, e.key->spritesheet->sheet, &src, &dest, 0, NULL, SDL_FLIP_NONE);
	else SDL_RenderCopyEx(renderer, e.key->spritesheet->sheet, &src, &dest, 0, NULL, SDL_FLIP_HORIZONTAL);
}

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
	char prev_t_state = 0;
	char prev_space_state = 0;
	int input_order[5] = { 0 };
	int n_inputs = 0;
	float atk_range = 0.25;
	SDL_Texture* hud_sheet = { 0 };
	load_Image_To_Texture(&hud_sheet, renderer, "Assets/Images/HUD_Elements.png");

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
	int player_money = 0;

	Sheet enemy_sheet = { 0 };
	load_Sheet(&enemy_sheet, 64, 64, renderer, "Assets/Images/EnemySpritesheet.png");

	Entity_Key enemy_key = { 0 };
	load_Entity_Key(&enemy_key, "Assets/EDS/Enemy.eds", &enemy_sheet);
	enemy_key.speed = 0.05f;
	enemy_key.max_hp = 2;

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
					else if (key == SDLK_w)
					{
						++n_inputs;
						input_order[UP] = n_inputs;
						w_state = 1;
					}
					else if (key == SDLK_a)
					{
						++n_inputs;
						input_order[LEFT] = n_inputs;
						a_state = 1;
					}
					else if (key == SDLK_s)
					{
						++n_inputs;
						input_order[DOWN] = n_inputs;
						s_state = 1;
					}
					else if (key == SDLK_d)
					{
						++n_inputs;
						input_order[RIGHT] = n_inputs;
						d_state = 1;
					}
					else if (key == SDLK_t) t_state = 1;
					else if (key == SDLK_SPACE) space_state = 1;
				}
				else if (event.type == SDL_KEYUP)
				{
					char key = event.key.keysym.sym;
					if (key == SDLK_w)
					{
						int order_pos = input_order[UP];
						input_order[UP] = 0;
						for (int i = 1; i < 5; ++i) if (input_order[i] > order_pos) --input_order[i];
						w_state = 0;
					}
					else if (key == SDLK_a)
					{
						int order_pos = input_order[LEFT];
						input_order[LEFT] = 0;
						for (int i = 1; i < 5; ++i) if (input_order[i] > order_pos) --input_order[i];
						a_state = 0;
					}
					else if (key == SDLK_s)
					{
						int order_pos = input_order[DOWN];
						input_order[DOWN] = 0;
						for (int i = 1; i < 5; ++i) if (input_order[i] > order_pos) --input_order[i];
						s_state = 0;
					}
					else if (key == SDLK_d)
					{
						int order_pos = input_order[RIGHT];
						input_order[RIGHT] = 0;
						for (int i = 1; i < 5; ++i) if (input_order[i] > order_pos) --input_order[i];
						d_state = 0;
					}
					else if (key == SDLK_t) t_state = 0;
					else if (key == SDLK_SPACE) space_state = 0;
				}
			}

			int last_pressed = 0;
			for (int i = 1; i < 5; ++i) if (input_order[i] > input_order[last_pressed]) last_pressed = i;

			if (t_state == 1 && prev_t_state == 0) debug_toggle *= -1;

			//input physics
			if (direction == MID)
			{
				if (space_state == 1 && prev_space_state == 0 && (p1.state == IDLE || p1.state == WALK)) p1.state = ATTACK;
				else
				{
					if (p1.state != ATTACK && p1.state != DAMAGE && p1.state != DYING && p1.state != DEAD)
					{
						p1.state = IDLE;
						if (last_pressed == LEFT)
						{
							p1.state = WALK;
							p1.direction = LEFT;
							p1.x -= p1.key->speed;
						}
						else if (last_pressed == RIGHT)
						{
							p1.state = WALK;
							p1.direction = RIGHT;
							p1.x += p1.key->speed;
						}
						else if (last_pressed == DOWN)
						{
							p1.state = WALK;
							p1.direction = DOWN;
							p1.y += p1.key->speed;
						}
						else if (last_pressed == UP)
						{
							p1.state = WALK;
							p1.direction = UP;
							p1.y -= p1.key->speed;
						}
					}
					else if (p1.state == DEAD)
					{
						n_enemies = 0;
						for (int i = 0; i < max_enemies; ++i) enemies[i].state = DEAD;
						p1.state = IDLE;
						p1.curr_hp = p1.key->max_hp;
						p1.x = dungeon_map.room_w * 0.5 - 0.5;
						p1.y = dungeon_map.room_h * 0.5 - 0.5;
						player_money = 0;
						locale[MID] = generate_Map(&dungeon_map, 3, 3, key);
						locale[LEFT] = locale[MID] - 1;
						locale[RIGHT] = locale[MID] + 1;
						locale[DOWN] = locale[MID] + dungeon_map.map_w;
						locale[UP] = locale[MID] - dungeon_map.map_w;
						++dungeon_map.visited[locale[MID]];
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
				
				//AI behaviour
				for (int i = 0; i < max_enemies; ++i)
				{
					if (enemies[i].state != DAMAGE && enemies[i].state != DYING && enemies[i].state != DEAD)
					{
						move_To_Position(&enemies[i], dungeon_map.wall_collisions[locale[MID]], dungeon_map.floor_collisions[locale[MID]], dungeon_map.room_w, dungeon_map.room_h, p1.x, p1.y);
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
							if (check_Hit_Collision(&enemies[i], p1, atk_range) == 2) ++n_hits;
						}
					}
					player_money += n_hits * 10;
					n_enemies -= n_hits;
				}
				if (p1.state != DEAD && p1.state != DAMAGE && p1.state != DYING)
				{
					for (int i = 0; i < max_enemies; ++i)
					{
						if (enemies[i].state == ATTACK)
						{
							int facing = F_SIDE;
							if (enemies[i].direction == DOWN) facing = F_DOWN;
							else if (enemies[i].direction == UP) facing = F_UP;
							if (enemies[i].curr_frame > enemies[i].key->atk_start[facing] + 1 && enemies[i].curr_frame <= enemies[i].key->atk_end[facing])
							{
								check_Hit_Collision(&p1, enemies[i], atk_range);
							}
						}
					}
				}

				//wall collisions
				check_Tile_Collision(dungeon_map.wall_collisions[locale[MID]], dungeon_map.floor_collisions[locale[MID]], dungeon_map.room_w, &p1, n_enemies);
				for (int i = 0; i < max_enemies; ++i)
				{
					if (enemies[i].state != DEAD)
					{
						check_Tile_Collision(dungeon_map.wall_collisions[locale[MID]], dungeon_map.floor_collisions[locale[MID]], dungeon_map.room_w, &enemies[i], n_enemies);
					}
				}
			}
			else
			{
				//camera room panning
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
						p1.y = (float)dungeon_map.room_h - 3.0f;
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
						p1.y = 2.0f;
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
						p1.x = dungeon_map.room_w - 3.0f;
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
						p1.x = 2.0f;
						direction = MID;
					}
				}

				//spawn enemies in unvisitted room
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
			SDL_Rect bar_src = { 192, 0, 64, 128 };
			SDL_Rect bar_dest = { 0, 0, 64, 128 };
			//SDL_RenderCopy(renderer, );
			//display money
			Text::draw_Text(renderer, "Money: ", 0, 0, 250, &font, 21);
			Text::draw_Int(renderer, player_money, 140, 0, 250, &font, 21);
			SDL_RenderPresent(renderer);
		}
	}
}