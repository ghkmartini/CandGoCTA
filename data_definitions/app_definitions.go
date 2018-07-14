package data_definitions

//Opt for real-time (1x or 10x time-lapse) or processor-limited simulation
type ProcessingType int

const (
	REAL_TIME_1X      = 0 //Real time, 1 elapsed second means 1 second of traffic simulation
	REAL_TIME_10X     = 1 //Real time, 1 elapsed second means 10 second of traffic simulation
	PROCESSOR_LIMITED = 2 //Run the simulation as fast as possible, limited to the computer capacity
)

const Processing_type ProcessingType = PROCESSOR_LIMITED

//Two test scenarios
type TestScenario int

const (
	SCENARIO_1 = 0 //Scenario 1, constant turn rate, 1 lane per street
	SCENARIO_2 = 1 //Scenario 2, turn rate of 10-35%, 1-4 lane
)

const Test_scenario TestScenario = SCENARIO_2

//Two control strategies
type ControStrategy int

const (
	IC_CONTROL   = 0 //Traffic lights are independent with their own timers
	CBCL_CONTROL = 1 //Traffic lights change operation based on block occupation
)

const Control_strategy ControStrategy = CBCL_CONTROL

//Multi-threaded control
type ThreadControl int

const (
	SINGLE_THREAD     = 0 //2s
	MULTI_THREAD      = 1 //1.5s
	MULTI_THREAD_POOL = 2 //Ver sync
)

const Thread_control ThreadControl = MULTI_THREAD_POOL

//Types of syncing between threads
type ThreadSync int

const (
	HARD_SYNC = 0 //1.7s
	SOFT_SYNC = 1 //1.6s
	NO_SYNC   = 2 //1.3s
)

const Thread_sync ThreadSync = NO_SYNC

//Traffic intensity
type TrafficIntensity int

const (
	CAR_RATE_LINEAR_1      = 1 //The number means 0.1cars/lane/entrypoint/second
	CAR_RATE_LINEAR_2      = 2 //The number means 0.2cars/lane/entrypoint/second
	CAR_RATE_LINEAR_3      = 3 //The number means 0.3cars/lane/entrypoint/second
	CAR_RATE_LINEAR_4      = 4 //The number means 0.4cars/lane/entrypoint/second
	CAR_RATE_LINEAR_5      = 5 //The number means 0.5cars/lane/entrypoint/second
	CAR_RATE_LINEAR_GROWTH = 6 //Starts with 0.1cars/lane/entrypoint/second and finishes at 0.5cars/lane/entrypoint/second
	CAR_RATE_POISSON       = 7 //Does a simplified poisson distribution of car rate input. From 0.1 to a peak of 0.5cars/lane/entrypoint/second
)

const Traffic_intensity TrafficIntensity = CAR_RATE_POISSON

//Simulation lenght - street.h dependencies
const Sim_length int = (2000 * 1000)    //Time in ms for the simulation - Not to exceed 2000s without increasing car rates on app_definitions.c
const Car_size int = 4                  //Car size in meters
const N_streets_w_e int = 10            //Amount of streets from West to East - Not to exceed 10 without increasing lane counts on app_definitions.c
const N_streets_n_s int = N_streets_w_e //Amount of streets from North to South - Not to exceed 10 without increasing lane counts on app_definitions.c
const Block_size int = 100              //Length of block in meters REQ: E
const Max_lanes int = 4                 //Max number of lanes per street

//light.h dependencies           //REQ: O
const Lights_red_time int = 47   //Time in seconds in red
const Lights_green_time int = 38 //Time in seconds in green
const Lights_yellow_time int = 5 //Time in seconds in yellow
const Lights_dead_time int = 1   //Time in seconds where both lights are red

//CBCL control definitions
const Lights_red_time_reduce_red int = 24 //Time to make traffic light cycle shorter
const Lights_red_time_no_change int = 39  //Time to avoid changing since it is close from being green
const Lights_red_time_reduced int = 30    //Time in seconds for red lights when block has 60% or more occupation
