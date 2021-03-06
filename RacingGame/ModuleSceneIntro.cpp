#include "Globals.h"
#include "Application.h"
#include "ModuleSceneIntro.h"
#include "Primitive.h"
#include "PhysBody3D.h"

ModuleSceneIntro::ModuleSceneIntro(bool start_enabled) : Module(start_enabled)
{
	//pizza positions
	pizza_position[0] = { 150, 1.1f, 150 }; // top left corner
	pizza_position[1] = { -25, 1.1f, -152.5f }; //mid bottom
	pizza_position[2] = { 150, 1.1f, -40.f }; //mid-left
	pizza_position[3] = { -150, 1.1f, 150 }; //top right corner
	pizza_position[4] = { 150, 1.1f, -150 }; //bottom left corner
	pizza_position[5] = { -145, 1.1f, 0 }; //mid right
	pizza_position[6] = { 95, 1.1f, 85 }; //corner left
	pizza_position[7] = { 25, 1.1f, -37.5f }; //center
	pizza_position[8] = { -150, 1.1f, -150 }; // bottom right corner
	pizza_position[9] = { -67.5f, 1.1f, 82.5f }; //corner right

	bollard_change_time = 5;
	starting_fx_time = 3;
	play_music = false;
	bollards_A_up = true;
	winning_map_created = false;
	showing_winning_map = false;
	p = 0;
}

ModuleSceneIntro::~ModuleSceneIntro()
{}

// Load assets
bool ModuleSceneIntro::Start()
{
	LOG("Loading Intro assets");
	bool ret = true;
	int k = 0;

	start_timer.Start();

	start = App->audio->LoadFx("Assets/Start.wav");
	App->audio->PlayFx(start);
	App->camera->Move(vec3(1.0f, 1.0f, 0.0f));
	bollard_timer.Start();

	CreatePizza();
	CreateBollards();
	CreateDecoration();
	CreateBuildings();

	//Save initial position
	Save();
	return ret;
}

// Load assets
bool ModuleSceneIntro::CleanUp()
{
	LOG("Unloading Intro scene");

	DestroyWinningMap();

	for (int i = 0; i < bollards_c.Count(); i++)
	{
		bollards_c[i]->~btSliderConstraint();
	}

	for(int i = 0; i < primitives.Count(); i++)
	{
		delete primitives[i];
	}

	return true;
}

void ModuleSceneIntro::HandleDebugInput()
{
	if (App->input->GetKey(SDL_SCANCODE_1) == KEY_DOWN)
		DebugSpawnPrimitive(new Sphere());
	if (App->input->GetKey(SDL_SCANCODE_2) == KEY_DOWN)
		DebugSpawnPrimitive(new Cube());
	if (App->input->GetKey(SDL_SCANCODE_3) == KEY_DOWN)
		DebugSpawnPrimitive(new Cylinder());
	if (App->input->GetKey(SDL_SCANCODE_4) == KEY_DOWN)
		for (uint n = 0; n < primitives.Count(); n++)
			primitives[n]->SetPos((float)(std::rand() % 40 - 20), 10.f, (float)(std::rand() % 40 - 20));
	if (App->input->GetKey(SDL_SCANCODE_5) == KEY_DOWN)
		for (uint n = 0; n < primitives.Count(); n++)
			primitives[n]->body.Push(vec3((float)(std::rand() % 500) - 250, 500, (float)(std::rand() % 500) - 250));

	if (App->input->GetMouseButton(SDL_BUTTON_LEFT) == KEY_DOWN)
	{
		//Get a vector indicating the direction from the camera viewpoint to the "mouse"
		const vec2 mousePos(((float)App->input->GetMouseX() / (float)App->window->Width()) * 2.f - 1.f,
			-((float)App->input->GetMouseY() / (float)App->window->Height()) * 2.f + 1.f);
		const vec4 rayEye = inverse(App->renderer3D->ProjectionMatrix) * vec4(mousePos.x, mousePos.y, -1.f, 1.f);
		const vec4 rayWorld(inverse(App->camera->GetViewMatrix()) * vec4(rayEye.x, rayEye.y, -1.f, 0.f));

		vec3 Dir(rayWorld.x, rayWorld.y, rayWorld.z);
		//Cast a ray from the camera, in the "mouse" direction
		PhysBody3D* body = App->physics->RayCast(App->camera->Position, Dir);
		if (body)
		{
			//Change the color of the clicked primitive
			body->parentPrimitive->color = Color((float)(std::rand() % 255) / 255.f, (float)(std::rand() % 255) / 255.f, (float)(std::rand() % 255) / 255.f);
		}
	}
}

void ModuleSceneIntro::DebugSpawnPrimitive(Primitive * p)
{
	primitives.PushBack(p);
	p->SetPos(App->camera->Position.x, App->camera->Position.y, App->camera->Position.z);
	p->body.collision_listeners.PushBack(this);
	p->body.Push(-App->camera->Z * 1000.f);
}

// Update
update_status ModuleSceneIntro::Update(float dt)
{

	if (App->debug == true)
		HandleDebugInput();

	for (uint n = 0; n < primitives.Count(); n++)
		primitives[n]->Update();

	if ((start_timer.Read() >= starting_fx_time * 1000)&&(start_timer.Read() < (starting_fx_time + 1) * 1000))
	{
		play_music = true;
		start_timer.Stop();
		App->player->timer.Start();
	}

	if (play_music)
	{
		App->audio->PlayMusic("Assets/Italian_music.ogg");
		App->audio->VolumeMusic(110);
		play_music = false;
	}

	if (bollard_timer.Read() > bollard_change_time * 1000) {
		bollard_timer.Start();
		if (bollards_A_up) { bollards_A_up = false; }
		else
		{
			bollards_A_up = true;
		}
	}

	if(bollards_A_up){
		for (int i = 0; i < bollards_A.Count(); i++) {
			bollards_A[i]->body.Push(vec3(0, 100000, 0));
		}
	}
	else
	{
		for (int i = 0; i < bollards_B.Count(); i++) {
			bollards_B[i]->body.Push(vec3(0, 100000, 0));
		}
	}

	if ((p >= 9)&&(!winning_map_created))
	{
		CreateWinningMap();
		winning_map_created = true;
		App->window->SetTitle("You Won! Press R to Restart ");
		showing_winning_map = true;
	}
	if (showing_winning_map)
	{
		App->camera->LookAt(vec3(400, 0, 400));
		for (int i = 0; i < winning_primitives.Count(); i++)
		{
			winning_primitives[i]->Update();
			winning_primitives[i]->Render();
		}
	}

	return UPDATE_CONTINUE;
}

update_status ModuleSceneIntro::PostUpdate(float dt)
{
	for (uint n = 0; n < primitives.Count(); n++)
	{
		primitives[n]->Render();
	}

	return UPDATE_CONTINUE;
}

void ModuleSceneIntro::OnCollision(PhysBody3D * body1, PhysBody3D * body2)
{
	Color color = Color((float)(std::rand() % 255) / 255.f, (float)(std::rand() % 255) / 255.f, (float)(std::rand() % 255) / 255.f);

	//body1->parentPrimitive->color = color;
	//body2->parentPrimitive->color = color;
}

void ModuleSceneIntro::Save()
{
	saved_position.x = App->player->position.x;
	saved_position.y = App->player->position.y;
	saved_position.z = App->player->position.z;

	pizzas_collected = p;
}

void ModuleSceneIntro::Load()
{
	App->player->vehicle->SetPos(saved_position.x, saved_position.y, saved_position.z);

	App->player->vehicle->GetBody()->setLinearVelocity({ 0,0,0 });
	App->player->acceleration = 0;

	p = pizzas_collected;
}

void ModuleSceneIntro::CreateBuildings()
{
	//Collider limit buildings
	{
		//Map limits at (x,y,z) = (175, y, 180)
		{//Map limits front (x,y,180)
			Cube* front_limit_building0 = new Cube({ 100, 40, 30 }, 10000);
			front_limit_building0->color = Darker_Red;
			front_limit_building0->SetPos(140, 0, 180);
			primitives.PushBack(front_limit_building0);

			Cube* front_limit_building1 = new Cube({ 70, 70, 30 }, 10000);
			front_limit_building1->color = Darker_Red;
			front_limit_building1->SetPos(60, 0, 180);
			primitives.PushBack(front_limit_building1);

			Cube* front_limit_building2 = new Cube({ 100, 50, 30 }, 10000);
			front_limit_building2->color = Darker_Red;
			front_limit_building2->SetPos(-20, 0, 180);
			primitives.PushBack(front_limit_building2);

			Cube* front_limit_building3 = new Cube({ 50, 100, 30 }, 10000);
			front_limit_building3->color = Darker_Red;
			front_limit_building3->SetPos(-100, 50, 170);
			primitives.PushBack(front_limit_building3);

			Cube* front_limit_building4 = new Cube({ 50, 70, 30 }, 10000);
			front_limit_building4->color = Darker_Red;
			front_limit_building4->SetPos(-130, 0, 180);
			primitives.PushBack(front_limit_building4);
		}
		{//Map limits left (175,y,z)
			Cube* left_limit_building0 = new Cube({ 30, 70, 30 }, 10000);
			left_limit_building0->color = Dark_Green;
			left_limit_building0->SetPos(175, 0, 150);
			primitives.PushBack(left_limit_building0);

			Cube* left_limit_building1 = new Cube({ 30, 50, 60 }, 10000);
			left_limit_building1->color = Dark_Green;
			left_limit_building1->SetPos(175, 0, 105);
			primitives.PushBack(left_limit_building1);

			Cube* left_limit_building2 = new Cube({ 30, 40, 100 }, 10000);
			left_limit_building2->color = Dark_Green;
			left_limit_building2->SetPos(175, 0, 25);
			primitives.PushBack(left_limit_building2);

			Cube* left_limit_building3 = new Cube({ 30, 70, 30 }, 10000);
			left_limit_building3->color = Dark_Green;
			left_limit_building3->SetPos(175, 0, -40);
			primitives.PushBack(left_limit_building3);

			Cube* left_limit_building4 = new Cube({ 30, 60, 120 }, 10000);
			left_limit_building4->color = Dark_Green;
			left_limit_building4->SetPos(175, 0, -95);
			primitives.PushBack(left_limit_building4);
		}
		{//Map limits right
			Cube* right_limit_building0 = new Cube({ 30, 70, 30 }, 10000);
			right_limit_building0->color = Beige;
			right_limit_building0->SetPos(-175, 0, 150);
			primitives.PushBack(right_limit_building0);

			Cube* right_limit_building1 = new Cube({ 30, 50, 60 }, 10000);
			right_limit_building1->color = Beige;
			right_limit_building1->SetPos(-175, 0, 105);
			primitives.PushBack(right_limit_building1);

			Cube* right_limit_building2 = new Cube({ 30, 40, 158 }, 10000);
			right_limit_building2->color = Beige;
			right_limit_building2->SetPos(-170, 0, 0);
			primitives.PushBack(right_limit_building2);

			Cube* right_limit_building4 = new Cube({ 30, 60, 90 }, 10000);
			right_limit_building4->color = Beige;
			right_limit_building4->SetPos(-175, 0, -125);
			primitives.PushBack(right_limit_building4);
		}
		{//Map limits back
			Cube* back_limit_building0 = new Cube({ 100, 40, 30 }, 10000);
			back_limit_building0->color = Darker_Grey;
			back_limit_building0->SetPos(140, 0, -180);
			primitives.PushBack(back_limit_building0);

			Cube* back_limit_building1 = new Cube({ 70, 70, 30 }, 10000);
			back_limit_building1->color = Darker_Grey;
			back_limit_building1->SetPos(60, 0, -180);
			primitives.PushBack(back_limit_building1);

			Cube* back_limit_building2 = new Cube({ 100, 50, 30 }, 10000);
			back_limit_building2->color = Darker_Grey;
			back_limit_building2->SetPos(-20, 0, -180);
			primitives.PushBack(back_limit_building2);

			Cube* back_limit_building3 = new Cube({ 50, 100, 30 }, 10000);
			back_limit_building3->color = Darker_Grey;
			back_limit_building3->SetPos(-100, 50, -180);
			primitives.PushBack(back_limit_building3);

			Cube* back_limit_building4 = new Cube({ 50, 70, 30 }, 10000);
			back_limit_building4->color = Darker_Grey;
			back_limit_building4->SetPos(-130, 0, -180);
			primitives.PushBack(back_limit_building4);
		}
	}
	//City buildings 1
		{
			Cube* city_building1 = new Cube({ 30, 40, 40 }, 10000);
			city_building1->color = Dark_Green;
			city_building1->SetPos(120, 0, 100);
			primitives.PushBack(city_building1);

			Cube* city_building2 = new Cube({ 30, 50, 30 }, 10000);
			city_building2->color = Darker_Grey;
			city_building2->SetPos(120, 0, 70);
			primitives.PushBack(city_building2);

			Cube* city_building3 = new Cube({ 30, 30, 80 }, 10000);
			city_building3->color = Darker_Red;
			city_building3->SetPos(120, 0, 30);
			primitives.PushBack(city_building3);

			Cube* city_building4 = new Cube({ 30, 30, 30 }, 10000);
			city_building4->color = Beige;
			city_building4->SetPos(120, 0, -70);
			primitives.PushBack(city_building4);

			Cube* city_building5 = new Cube({ 30, 70, 50 }, 10000);
			city_building5->color = Darker_Red;
			city_building5->SetPos(100, 0, -110);
			primitives.PushBack(city_building5);
		}


	//City buildings 2
		{
			Cube* city_building6 = new Cube({ 40, 30, 50 }, 10000);
			city_building6->color = Darker_Grey;
			city_building6->SetPos(100, 0, -110);
			primitives.PushBack(city_building6);

			Cube* city_building7 = new Cube({ 55, 50, 50 }, 10000);
			city_building7->color = Dark_Green;
			city_building7->SetPos(15, 0, -110);
			primitives.PushBack(city_building7);

			Cube* city_building8 = new Cube({ 90, 70, 50 }, 10000);
			city_building8->color = Darker_Red;
			city_building8->SetPos(-80, 0, -110);
			primitives.PushBack(city_building8);
		}

	//City buildings 3

		{
			Cube* city_building9 = new Cube({ 50, 30, 70 }, 10000);
			city_building9->color = Dark_Green;
			city_building9->SetPos(-100, 0, -50);
			primitives.PushBack(city_building9);

			Cube* city_building10 = new Cube({ 50, 30, 50 }, 10000);
			city_building10->color = Darker_Red;
			city_building10->SetPos(-100, 0, 40);
			primitives.PushBack(city_building10);

			Cube* city_building11 = new Cube({ 50, 50, 60 }, 10000);
			city_building11->color = Darker_Grey;
			city_building11->SetPos(-100, 0, 90);
			primitives.PushBack(city_building11);
		}


	//City buildings 4

		{
			Cube* city_building12 = new Cube({ 50, 30, 30 }, 10000);
			city_building12->color = Beige;
			city_building12->SetPos(-50, 0, 110);
			primitives.PushBack(city_building12);

			Cube* city_building13 = new Cube({ 20, 40, 30 }, 10000);
			city_building13->color = Dark_Grey;
			city_building13->SetPos( -20, 0, 110);
			primitives.PushBack(city_building13);

			Cube* city_building14 = new Cube({ 50, 50, 30 }, 10000);
			city_building14->color = Dark_Green;
			city_building14->SetPos( 50, 0, 110);
			primitives.PushBack(city_building14);
		}


	//City buildings 5
		{
			Cube* city_building15 = new Cube({ 20, 30, 40 }, 10000);
			city_building15->color = Darker_Red;
			city_building15->SetPos(-40, 0, -40);
			primitives.PushBack(city_building15);

			Cube* city_building16 = new Cube({ 20, 50, 60 }, 10000);
			city_building16->color = Darker_Grey;
			city_building16->SetPos(-35, 0, 40);
			primitives.PushBack(city_building16);
		}

	//City buildings 6

		{
			Cube* city_building17 = new Cube({ 20, 10, 20 }, 10000);
			city_building17->color = Dark_Green;
			city_building17->SetPos(-20, 0, 60);
			primitives.PushBack(city_building17);

			Cube* city_building18 = new Cube({ 20, 20, 20 }, 10000);
			city_building18->color = Beige;
			city_building18->SetPos( 0, 0, 60);
			primitives.PushBack(city_building18);

			Cube* city_building19 = new Cube({ 20, 30, 20 }, 10000);
			city_building19->color = Dark_Green;
			city_building19->SetPos(40, 0, 60);
			primitives.PushBack(city_building19);

			Cube* city_building20 = new Cube({ 25, 40, 20 }, 10000);
			city_building20->color = Darker_Red;
			city_building20->SetPos( 60, 0, 60);
			primitives.PushBack(city_building20);
		}


	//City buildings 7
		{
			Cube* city_building21 = new Cube({ 20, 20, 20 }, 10000);
			city_building21->color = Beige;
			city_building21->SetPos(100, 0, 20);
			primitives.PushBack(city_building21);

			Cube* city_building22 = new Cube({ 20, 40, 20 }, 10000);
			city_building22->color = Dark_Green;
			city_building22->SetPos(80, 0, 20);
			primitives.PushBack(city_building22);

			Cube* city_building23 = new Cube({ 20, 30, 20 }, 10000);
			city_building23->color = Darker_Red;
			city_building23->SetPos(60, 0, 20);
			primitives.PushBack(city_building23);

			Cube* city_building24 = new Cube({ 25, 40, 20 }, 10000);
			city_building24->color = Beige;
			city_building24->SetPos(40, 0, 20);
			primitives.PushBack(city_building24);

			Cube* city_building25 = new Cube({ 20, 40, 20 }, 10000);
			city_building25->color = Darker_Red;
			city_building25->SetPos(20, 0, 20);
			primitives.PushBack(city_building25);

			Cube* city_building26 = new Cube({ 20, 20, 20 }, 10000);
			city_building26->color = Dark_Green;
			city_building26->SetPos(-20, 0, 20);
			primitives.PushBack(city_building26);
		}

	//City buildings 8
		{
			Cube* city_building27 = new Cube({ 20, 20, 20 }, 10000);
			city_building27->color = Beige;
			city_building27->SetPos( 0, 0, -20);
			primitives.PushBack(city_building27);

			Cube* city_building28 = new Cube({ 20, 20, 10 }, 10000);
			city_building28->color = Dark_Green;
			city_building28->SetPos(0, 0, -50);
			primitives.PushBack(city_building28);

			Cube* city_building29 = new Cube({ 20, 20, 20 }, 10000);
			city_building29->color = Darker_Red;
			city_building29->SetPos(50, 0, -20);
			primitives.PushBack(city_building29);

			Cube* city_building30 = new Cube({ 20, 20, 10 }, 10000);
			city_building30->color = Darker_Grey;
			city_building30->SetPos(50, 0, -50);
			primitives.PushBack(city_building30);

		}

}

void ModuleSceneIntro::CreatePizza()
{
	float tape_angle = 60.f;

	pizza.base = new Cube({ 2, 0.2f, 2 }, 0);
	primitives.PushBack(pizza.base);
	pizza.base->color = Beige;
	pizza.base->body.SetAsSensor(true);
	pizza.base->body.collision_listeners.PushBack(this);

	pizza.tape = new Cube({ 2, 0.2f, 2 }, 0);
	pizza.tape->transform.rotate(tape_angle, vec3(0, 0, 1));
	primitives.PushBack(pizza.tape);
	pizza.tape->color = Beige;
	pizza.tape->body.SetAsSensor(true);
	pizza.tape->body.collision_listeners.PushBack(this);

	pizza.pizza = new Cylinder(0.8, 0.1f, 0);
	pizza.pizza->transform.rotate(90.f, vec3(0, 0, 1));
	primitives.PushBack(pizza.pizza);
	pizza.pizza->color = Red;
	pizza.pizza->body.SetAsSensor(true);
	pizza.pizza->body.collision_listeners.PushBack(this);
	pizza.pizza->body.isPizza = true;

	//We set the pizza position
	changePizzaPosition();
}

void ModuleSceneIntro::CreateDecoration()
{
	{
		Cube* sidewalk1 = new Cube({ 38, 0.5f, 160 }, 0);
		sidewalk1->color = Bright_Grey;
		sidewalk1->SetPos(120, 0, 55);
		primitives.PushBack(sidewalk1);

		Cube* sidewalk2 = new Cube({ 41, 0.5f, 34 }, 0);
		sidewalk2->color = Bright_Grey;
		sidewalk2->SetPos(120, 0, -66);
		primitives.PushBack(sidewalk2);
	}
	{
		Cube* sidewalk3 = new Cube({ 77, 0.5f, 55 }, 0);
		sidewalk3->color = Bright_Grey;
		sidewalk3->SetPos(102, 0, -110);
		primitives.PushBack(sidewalk3);

		Cube* sidewalk4 = new Cube({ 60, 0.5f, 55 }, 0);
		sidewalk4->color = Grey;
		sidewalk4->SetPos(15, 0, -110);
		primitives.PushBack(sidewalk4);

		Cube* sidewalk5 = new Cube({ 95, 0.5f, 55 }, 0);
		sidewalk5->color = Bright_Grey;
		sidewalk5->SetPos(-80, 0, -110);
		primitives.PushBack(sidewalk5);
	}
	{
		Cube* sidewalk6 = new Cube({ 55, 0.5f, 73 }, 0);
		sidewalk6->color = Bright_Grey;
		sidewalk6->SetPos(-100, 0, -46);
		primitives.PushBack(sidewalk6);

		Cube* sidewalk7 = new Cube({ 55, 0.5f, 117 }, 0);
		sidewalk7->color = Bright_Grey;
		sidewalk7->SetPos(-100, 0, 68);
		primitives.PushBack(sidewalk7);
	}
	{
		Cube* sidewalk8 = new Cube({ 72, 0.5f, 35 }, 0);
		sidewalk8->color = Bright_Grey;
		sidewalk8->SetPos(-36.5f, 0, 110);
		primitives.PushBack(sidewalk8);

		Cube* sidewalk9 = new Cube({ 55, 0.5f,35 }, 0);
		sidewalk9->color = Bright_Grey;
		sidewalk9->SetPos(50, 0, 110);
		primitives.PushBack(sidewalk9);
	}
	{
		Cube* sidewalk10 = new Cube({ 25, 0.5f, 45 }, 0);
		sidewalk10->color = Bright_Grey;
		sidewalk10->SetPos(-40, 0, -40);
		primitives.PushBack(sidewalk10);

		Cube* sidewalk11 = new Cube({ 25, 0.5f, 65 }, 0);
		sidewalk11->color = Bright_Grey;
		sidewalk11->SetPos(-45, 0, 40);
		primitives.PushBack(sidewalk11);
	}
	{
		Cube* sidewalk12 = new Cube({ 45, 0.5f, 25 }, 0);
		sidewalk12->color = Bright_Grey;
		sidewalk12->SetPos(-10, 0, 60);
		primitives.PushBack(sidewalk12);

		Cube* sidewalk13 = new Cube({ 50, 0.5f, 25 }, 0);
		sidewalk13->color = Bright_Grey;
		sidewalk13->SetPos(52, 0, 60);
		primitives.PushBack(sidewalk13);
	}
	{
		Cube* sidewalk14 = new Cube({ 102, 0.5f, 25 }, 0);
		sidewalk14->color = Bright_Grey;
		sidewalk14->SetPos(50, 0, 20);
		primitives.PushBack(sidewalk14);

		Cube* sidewalk15 = new Cube({ 25, 0.5f, 25 }, 0);
		sidewalk15->color = Bright_Grey;
		sidewalk15->SetPos(-20, 0, 20);
		primitives.PushBack(sidewalk15);
	}
	{
		Cube* sidewalk16 = new Cube({ 25, 0.5f, 25 }, 0);
		sidewalk16->color = Bright_Grey;
		sidewalk16->SetPos(0, 0, -20);
		primitives.PushBack(sidewalk16);

		Cube* sidewalk17 = new Cube({ 25, 0.5f, 15 }, 0);
		sidewalk17->color = Bright_Grey;
		sidewalk17->SetPos(0, 0, -50);
		primitives.PushBack(sidewalk17);

		Cube* sidewalk18 = new Cube({ 25, 0.5f, 25 }, 0);
		sidewalk18->color = Bright_Grey;
		sidewalk18->SetPos(50, 0, -20);
		primitives.PushBack(sidewalk18);

		Cube* sidewalk19 = new Cube({ 25, 0.5f, 15 }, 0);
		sidewalk19->color = Bright_Grey;
		sidewalk19->SetPos(50, 0, -50);
		primitives.PushBack(sidewalk19);
	}
}

void ModuleSceneIntro::changePizzaPosition()
{
	if (p == 3 || p == 6)
		Save();

	pizza.base->SetPos(pizza_position[p].x, pizza_position[p].y - 0.2f, pizza_position[p].z);
	pizza.pizza->SetPos(pizza_position[p].x, pizza_position[p].y, pizza_position[p].z);
	pizza.tape->SetPos(pizza_position[p].x + 1.5f, pizza_position[p].y + 0.7f, pizza_position[p].z);
}

void ModuleSceneIntro::CreateSingleBollard(float x, float z, int group) {
	Cube* bollard;
	bollard = new Cube(vec3(0.5, 3, 0.5), 1000);
	primitives.PushBack(bollard);
	bollard->SetPos(x, 1.5f, z);
	bollard->color = Yellow;

	if(group == 1)
		bollards_A.PushBack(bollard);

	if (group == 2)
		bollards_B.PushBack(bollard);

	Cube* bollardBase;
	bollardBase = new Cube(vec3(0.8, 0.2, 0.8), 0);
	bollardBase->body.SetAsSensor(true);
	primitives.PushBack(bollardBase);
	bollardBase->SetPos(x, 0.2, z);
	bollardBase->color = Grey;

	btTransform frameInA, frameInB;

	btSliderConstraint* constraint;

	frameInA.setIdentity();
	frameInB.setIdentity();
	frameInA.getBasis().setEulerZYX(0, 0, M_PI_2);
	frameInB.getBasis().setEulerZYX(0, 0, M_PI_2);
	frameInA.setOrigin(btVector3(0.f, 0, 0.f));
	frameInB.setOrigin(btVector3(0.f, -2.6f, 0.f));

	constraint = App->physics->AddConstraintSlider(*bollard, *bollardBase, frameInA, frameInB);
	bollards_c.PushBack(constraint);
	constraint->setPoweredAngMotor(true);
	constraint->setTargetAngMotorVelocity(2);
	constraint->setLowerAngLimit(-0.1);
	constraint->setUpperAngLimit(0.1);

	constraint->setPoweredLinMotor(true);
	constraint->setTargetLinMotorVelocity(0);
	constraint->setLowerLinLimit(-9.6f);
	constraint->setUpperLinLimit(9.6f);
}

void ModuleSceneIntro::CreateBollards() {

	//initial position
	CreateSingleBollard(-2.5, 8,1);
	CreateSingleBollard(-5.5, 8,1);

	CreateSingleBollard(25, 50,2);
	CreateSingleBollard(22, 50,2);
	CreateSingleBollard(19, 50,2);
	CreateSingleBollard(16, 50,2);

	CreateSingleBollard(-17.5, -88.5,1);
	CreateSingleBollard(-21.5, -88.5,1);
	CreateSingleBollard(-25.5, -88.5,1);
	CreateSingleBollard(-29.5, -88.5,1);

	CreateSingleBollard(46.5, -88.5,2);
	CreateSingleBollard(50.5, -88.5,2);
	CreateSingleBollard(54.5, -88.5,2);
	CreateSingleBollard(58.5, -88.5,2);
	CreateSingleBollard(62.5, -88.5,2);

	CreateSingleBollard(-74, 7,2);
	CreateSingleBollard(-74, 2.5,2);
	CreateSingleBollard(-74, -2,2);
	CreateSingleBollard(-74, -6.5,2);

	CreateSingleBollard(20.5, 97.5,2);
	CreateSingleBollard(17.5, 97.5,2);
	CreateSingleBollard(14.5, 97.5,2);
	CreateSingleBollard(11.5, 97.5,2);
	CreateSingleBollard(8.5, 97.5,2);
	CreateSingleBollard(5.5, 97.5,2);
	CreateSingleBollard(2.5, 97.5,2);

	CreateSingleBollard(100, 97.5,1);
	CreateSingleBollard(97, 97.5,1);
	CreateSingleBollard(94, 97.5,1);
	CreateSingleBollard(91, 97.5,1);
	CreateSingleBollard(88, 97.5,1);
	CreateSingleBollard(85, 97.5,1);
	CreateSingleBollard(82, 97.5,1);
	CreateSingleBollard(79, 97.5,1);

	CreateSingleBollard(110, -27.5,2);
	CreateSingleBollard(110, -30.5,2);
	CreateSingleBollard(110, -33.5,2);
	CreateSingleBollard(110, -36.5,2);
	CreateSingleBollard(110, -39.5,2);
	CreateSingleBollard(110, -42.5,2);
	CreateSingleBollard(110, -45.5,2);
}

void ModuleSceneIntro::CreateWinningMap() {
	App->camera->Move(vec3(375, 20, 435));

	int object_numb = 150;
	float tape_angle = 60.f;

	Cube* winning_floor = new Cube(vec3(30, 0.1, 30), 0);
	winning_floor->SetPos(400, 0, 400);
	winning_primitives.PushBack(winning_floor);
	//winning_floor->color = Color((float)(std::rand() % 255) / 255.f, (float)(std::rand() % 255) / 255.f, (float)(std::rand() % 255) / 255.f);
	winning_floor->color = Color(0.33,0.45,0.18);

	//big pizza
	{
		Cube* base = new Cube({ 10, 1, 10 }, 0);
		winning_primitives.PushBack(base);
		base->color = Beige;
		base->SetPos(400, 2, 400);

		Cube* tape = new Cube({ 10, 1, 10 }, 0);
		tape->transform.rotate(tape_angle, vec3(0, 0, 1));
		winning_primitives.PushBack(tape);
		tape->color = Beige;
		tape->SetPos(407, 6, 400);

		Cylinder* pizza = new Cylinder(3.5, 0.2f, 0);
		pizza->transform.rotate(90.f, vec3(0, 0, 1));
		winning_primitives.PushBack(pizza);
		pizza->color = Red;
		pizza->SetPos(400, 2.7f, 400);

		Cylinder* pizza2 = new Cylinder(4, 0.3f, 0);
		pizza2->transform.rotate(90.f, vec3(0, 0, 1));
		winning_primitives.PushBack(pizza2);
		pizza2->color = Beige;
		pizza2->SetPos(400, 2.5, 400);

		Cylinder* pepperoni1 = new Cylinder(1, 0.2f, 0);
		pepperoni1->transform.rotate(90.f, vec3(0, 0, 1));
		winning_primitives.PushBack(pepperoni1);
		pepperoni1->color = Dark_Red;
		pepperoni1->SetPos(402, 2.8f, 400);

		Cylinder* pepperoni2 = new Cylinder(1, 0.2f, 0);
		pepperoni2->transform.rotate(90.f, vec3(0, 0, 1));
		winning_primitives.PushBack(pepperoni2);
		pepperoni2->color = Dark_Red;
		pepperoni2->SetPos(400, 2.8f, 402);

		Cylinder* pepperoni3 = new Cylinder(1, 0.2f, 0);
		pepperoni3->transform.rotate(90.f, vec3(0, 0, 1));
		winning_primitives.PushBack(pepperoni3);
		pepperoni3->color = Dark_Red;
		pepperoni3->SetPos(398, 2.8f, 400);

		Cylinder* pepperoni4 = new Cylinder(1, 0.2f, 0);
		pepperoni4->transform.rotate(90.f, vec3(0, 0, 1));
		winning_primitives.PushBack(pepperoni4);
		pepperoni4->color = Dark_Red;
		pepperoni4->SetPos(400, 2.8f, 398);
	}

	//you won
	{
		//Y
		{
			Cube* letters = new Cube({ 2, 6, 2 }, 0);
			letters->color = Green;
			letters->SetPos(389, 3, 391);
			winning_primitives.PushBack(letters);

			Cube* letters2 = new Cube({ 2, 4, 2 }, 0);
			letters2->transform.rotate(45.f, vec3(1, 0, 0));
			letters2->color = Green;
			letters2->SetPos(389, 5.5f, 392);
			winning_primitives.PushBack(letters2);

			Cube* letters3 = new Cube({ 2, 4, 2 }, 0);
			letters3->transform.rotate(-45.f, vec3(1, 0, 0));
			letters3->color = Green;
			letters3->SetPos(389, 5.5f, 390);
			winning_primitives.PushBack(letters3);

		}

		//O
		{
			Cube* letters4 = new Cube({ 2, 6, 2 }, 0);
			letters4->color = White;
			letters4->SetPos(389, 3, 396);
			winning_primitives.PushBack(letters4);

			Cube* letters5 = new Cube({ 2, 6, 2 }, 0);
			letters5->color = White;
			letters5->SetPos(389, 3, 400);
			winning_primitives.PushBack(letters5);

			Cube* letters6 = new Cube({ 2, 5, 2 }, 0);
			letters6->transform.rotate(90, vec3(1, 0, 0));
			letters6->color = White;
			letters6->SetPos(389, 1, 398);
			winning_primitives.PushBack(letters6);

			Cube* letters7 = new Cube({ 2, 5, 2 }, 0);
			letters7->transform.rotate(90, vec3(1, 0, 0));
			letters7->color = White;
			letters7->SetPos(389, 5, 398);
			winning_primitives.PushBack(letters7);
		}

		//U
		{
			Cube* letters8 = new Cube({ 2, 6, 2 }, 0);
			letters8->color = Red;
			letters8->SetPos(389, 3, 403);
			winning_primitives.PushBack(letters8);

			Cube* letters9 = new Cube({ 2, 6, 2 }, 0);
			letters9->color = Red;
			letters9->SetPos(389, 3, 407);
			winning_primitives.PushBack(letters9);

			Cube* letters10 = new Cube({ 2, 5, 2 }, 0);
			letters10->transform.rotate(90, vec3(1, 0, 0));
			letters10->color = Red;
			letters10->SetPos(389, 1, 405);
			winning_primitives.PushBack(letters10);

		}

		//W
		{
			Cube* letters11 = new Cube({ 2, 6, 2 }, 0);
			letters11->transform.rotate(30, vec3(0, 0, 1));
			letters11->color = Green;
			letters11->SetPos(392, 3.2, 409);
			winning_primitives.PushBack(letters11);

			Cube* letters12 = new Cube({ 2, 4, 2 }, 0);
			letters12->transform.rotate(-30, vec3(0, 0, 1));
			letters12->color = Green;
			letters12->SetPos(394, 2.5f, 409);
			winning_primitives.PushBack(letters12);

			Cube* letters13 = new Cube({ 2, 4, 2 }, 0);
			letters13->transform.rotate(30, vec3(0, 0, 1));
			letters13->color = Green;
			letters13->SetPos(396, 2.5f, 409);
			winning_primitives.PushBack(letters13);

			Cube* letters14 = new Cube({ 2, 6, 2 }, 0);
			letters14->transform.rotate(-30, vec3(0, 0, 1));
			letters14->color = Green;
			letters14->SetPos(398, 3.2, 409);
			winning_primitives.PushBack(letters14);

		}

		//O
		{
			Cube* letters15 = new Cube({ 2, 6, 2 }, 0);
			letters15->color = White;
			letters15->SetPos(402, 3, 409);
			winning_primitives.PushBack(letters15);

			Cube* letters16 = new Cube({ 2, 6, 2 }, 0);
			letters16->color = White;
			letters16->SetPos(406, 3, 409);
			winning_primitives.PushBack(letters16);

			Cube* letters17 = new Cube({ 2, 5, 2 }, 0);
			letters17->transform.rotate(90, vec3(0, 0, 1));
			letters17->color = White;
			letters17->SetPos(404.5f, 1, 409);
			winning_primitives.PushBack(letters17);

			Cube* letters18 = new Cube({ 2, 5, 2 }, 0);
			letters18->transform.rotate(90, vec3(0, 0, 1));
			letters18->color = White;
			letters18->SetPos(404.5f, 5, 409);
			winning_primitives.PushBack(letters18);
		}

		//N
		{
			Cube* letters19 = new Cube({ 2, 6, 2 }, 0);
			letters19->color = Red;
			letters19->SetPos(409, 3, 409);
			winning_primitives.PushBack(letters19);

			Cube* letters20 = new Cube({ 2, 6, 2 }, 0);
			letters20->color = Red;
			letters20->SetPos(413, 3, 409);
			winning_primitives.PushBack(letters20);

			Cube* letters21 = new Cube({ 2, 6, 2 }, 0);
			letters21->transform.rotate(37, vec3(0, 0, 1));
			letters21->color = Red;
			letters21->SetPos(411, 3, 409);
			winning_primitives.PushBack(letters21);

		}

	}


	for (int i = 0; i < object_numb; i++)
	{
		float size = std::rand() % 10;
		size = size * 0.1f;
		Sphere* ball = new Sphere(size);
		ball->SetPos((float)(std::rand() % 30) + 385, 35, (float)(std::rand() % 30) + 385);
		int color = std::rand() % 3;
		if (color == 0)
			ball->color = Red;
		if (color == 1)
			ball->color = White;
		if (color == 2)
			ball->color = Green;
		ball->body.Push(vec3((float)(std::rand() % 10), 0, (float)(std::rand() % 10)));
		ball->body.GetBody()->setRestitution(1);
		winning_primitives.PushBack(ball);
	}

}

void ModuleSceneIntro::DestroyWinningMap() {
	for (int i = 0; i < winning_primitives.Count(); i++)
	{
		winning_primitives[i]->body.~PhysBody3D();
	}
	p = 0;
	winning_primitives.Clear();
}
