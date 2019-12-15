#include "Globals.h"
#include "Application.h"
#include "ModulePlayer.h"
#include "Primitive.h"
#include "PhysVehicle3D.h"
#include "PhysBody3D.h"

ModulePlayer::ModulePlayer(bool start_enabled) : Module(start_enabled), vehicle(NULL)
{
	turn = acceleration = brake = 0.0f;
}

ModulePlayer::~ModulePlayer()
{}

// Load assets
bool ModulePlayer::Start()
{
	LOG("Loading player");

	VehicleInfo car;

	// Car properties ----------------------------------------
	car.mass = 500.0f;
	car.suspensionStiffness = 15.88f;
	car.suspensionCompression = 0.83f;
	car.suspensionDamping = 0.88f;
	car.maxSuspensionTravelCm = 1000.0f;
	car.frictionSlip = 50.5;
	car.maxSuspensionForce = 6000.0f;

	//Chassis
	car.chassis_size.Set(1, 1, 3.5);
	car.chassis_offset.Set(0, 1.0, 0);

	car.cabin_size.Set(1.1, 1.75, 1.25);
	car.cabin_offset.Set(0, 1.25, 1);

	car.trunk_size.Set(2, 1.95, 3.25);
	car.trunk_offset.Set(0, 1.55, -1.3);

	// Wheel properties ---------------------------------------
	float connection_height = 1.2f;
	float back_wheels_displacement = -0.4;
	float wheel_radius = 0.55f;
	float wheel_width = 0.38f;
	float suspensionRestLength = 1.2f;

	// Don't change anything below this line ------------------

	float half_width = car.chassis_size.x*0.5f;
	float half_length = car.chassis_size.z*0.5f;
	
	vec3 direction(0,-1,0);
	vec3 axis(-1,0,0);
	
	car.num_wheels = 3;
	car.wheels = new Wheel[3];

	// FRONT-LEFT ------------------------
	car.wheels[0].connection.Set(0, connection_height - 0.2, half_length - wheel_radius + 0.4f);
	car.wheels[0].direction = direction;
	car.wheels[0].axis = axis;
	car.wheels[0].suspensionRestLength = suspensionRestLength;
	car.wheels[0].radius = wheel_radius;
	car.wheels[0].width = wheel_width;
	car.wheels[0].front = true;
	car.wheels[0].drive = true;
	car.wheels[0].brake = false;
	car.wheels[0].steering = true;

	// REAR-LEFT ------------------------
	car.wheels[1].connection.Set(half_width + 1.2f * wheel_width, connection_height, -half_length + wheel_radius + back_wheels_displacement);
	car.wheels[1].direction = direction;
	car.wheels[1].axis = axis;
	car.wheels[1].suspensionRestLength = suspensionRestLength;
	car.wheels[1].radius = wheel_radius;
	car.wheels[1].width = wheel_width * 1.5f;
	car.wheels[1].front = false;
	car.wheels[1].drive = false;
	car.wheels[1].brake = true;
	car.wheels[1].steering = false;

	// REAR-RIGHT ------------------------
	car.wheels[2].connection.Set(-half_width - 1.2f * wheel_width, connection_height, -half_length + wheel_radius + back_wheels_displacement);
	car.wheels[2].direction = direction;
	car.wheels[2].axis = axis;
	car.wheels[2].suspensionRestLength = suspensionRestLength;
	car.wheels[2].radius = wheel_radius;
	car.wheels[2].width = wheel_width * 1.5f;
	car.wheels[2].front = false;
	car.wheels[2].drive = false;
	car.wheels[2].brake = true;
	car.wheels[2].steering = false;
	
	sensor = new Cube(5);
	sensor->body.collision_listeners.PushBack(this);
	sensor->body.SetAsSensor(true);

	vehicle = App->physics->AddVehicle(car);
	vehicle->SetPos(0, 0, 0);
	initial_position = vehicle->position;

	return true;
}

// Unload assets
bool ModulePlayer::CleanUp()
{
	LOG("Unloading player");

	return true;
}

// Update: draw background
update_status ModulePlayer::Update(float dt)
{
	turn = acceleration = brake = 0.0f;

	if((App->input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)&&(vehicle->GetKmh() < 120))
	{
		acceleration = MAX_ACCELERATION;
	}

	if(App->input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
	{
		if(turn < TURN_DEGREES)
			turn +=  TURN_DEGREES;
	}

	if(App->input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
	{
		if(turn > -TURN_DEGREES)
			turn -= TURN_DEGREES;
	}

	if(App->input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
	{
		acceleration = -MAX_ACCELERATION;
	}

	if (App->input->GetKey(SDL_SCANCODE_SPACE) == KEY_DOWN) {
		if (vehicle->HasBody())
		{
			vehicle->GetBody()->activate();
			vehicle->GetBody()->applyCentralForce(btVector3((float)1, (float)200, (float)1));
		}
	}

	vehicle->ApplyEngineForce(acceleration);
	vehicle->Turn(turn);
	vehicle->Brake(brake);

	position = vehicle->position;
	vec3 forward = vehicle->GetForwardVector();

	if (App->input->GetMouseButton(SDL_BUTTON_RIGHT) == KEY_REPEAT)
	{
		App->camera->LookAt(vec3(position.x, position.y, position.z));
	}
	else if (!App->debug)
	{
		App->camera->Position.Set(vehicle->position.x - forward.x * 10, 5.5f, vehicle->position.z - forward.z * 10);
		App->camera->LookAt(vec3(position.x, position.y + 1.5f, position.z));
	}

	sensor->Update();
	sensor->body.GetBody()->applyForce(btVector3(0,10,0),btVector3(0,0,0));
	sensor->SetPos(vehicle->position.x, 2, vehicle->position.z - 0.5);

	vehicle->Render();
	//sensor->Render();

	char title[80];
	sprintf_s(title, "%.1f Km/h", vehicle->GetKmh());
	App->window->SetTitle(title);

	return UPDATE_CONTINUE;
}

void ModulePlayer::OnCollision(PhysBody3D* body1, PhysBody3D* body2) {
	LOG("Collision");
}

void ModulePlayer::RestartGame() {
	vehicle->SetPos(initial_position.x, initial_position.y, initial_position.z);
	acceleration = -MAX_ACCELERATION;
}

