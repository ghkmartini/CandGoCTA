package street_data

import (
	"TrafficSimulatorV2/car_data"
	"TrafficSimulatorV2/data_definitions"
	"TrafficSimulatorV2/light_data"
	"math/rand"
	"sync"
)

const slots_per_block int = (data_definitions.Block_size / data_definitions.Car_size)
const max_blocks int = data_definitions.N_streets_w_e + 1
const max_slots int = (slots_per_block * max_blocks)

type street_direction int

const (
	NORTH = 0
	SOUTH = 1
	WEST  = 2
	EAST  = 3
)

type Street struct {
	street_number int
	block_count   int
	street_slots  [data_definitions.Max_lanes][max_slots]*car_data.Car

	Lane_count int
	Car_count  int
	Occupation [max_blocks]int
	Direction  street_direction //REQ: C
}

type Crossing struct {
	turn_rate   [2]int
	Street_w_e  *Street
	Street_n_s  *Street
	Cross_light *light_data.Light
}

var streets_w_e [data_definitions.N_streets_w_e]Street //REQ: A
var streets_n_s [data_definitions.N_streets_n_s]Street //REQ: B
var crossings [data_definitions.N_streets_w_e][data_definitions.N_streets_n_s]Crossing
var lights [light_data.N_lights]light_data.Light
var total_cars_on_map int
var jobs chan int

func Init_streets(directions int, n_streets int, lanes_in *[][]int, turn_rate_in *[][][]int) {

	total_cars_on_map = 0 //Set zero to the car counter to be calculated in the future

	for i := 0; i < light_data.N_lights; i++ {
		light_data.Init_light(&lights[i]) //Starts traffic lights with different time counters
	}

	var temp_street *[data_definitions.N_streets_w_e]Street

	for h := 0; h < 2; h++ { //Loop through north/south and west/east roads
		if h == 0 {
			temp_street = &streets_w_e //One direction at a time
		} else {
			temp_street = &streets_n_s
		}

		for i := 0; i < data_definitions.N_streets_w_e; i++ { //For every street in each direction

			temp_street[i].street_number = i                                //Set a street ID
			temp_street[i].block_count = data_definitions.N_streets_w_e + 1 //Set the amount of blocks - depends on the number of crossing streets
			temp_street[i].Lane_count = (*lanes_in)[h][i]                   //Set the number of lanes for this street

			if h == 0 {
				if (i % 2) == 0 {
					temp_street[i].Direction = WEST //Set the direction of the street, one to each direction, each time
				} else {
					temp_street[i].Direction = EAST // REQ: L
				}
			}

			if h == 1 {
				if (i % 2) == 0 {
					temp_street[i].Direction = NORTH //Set the direction of the street, one to each direction, each time
				} else {
					temp_street[i].Direction = SOUTH
				}
			}

			for j := 0; j < max_blocks; j++ {
				temp_street[i].Occupation[j] = 0 //Starts with zero occupation of cars on all blocks for every street
			}

			for j := 0; j < data_definitions.Max_lanes; j++ {
				for k := 0; k < max_slots; k++ {
					temp_street[i].street_slots[j][k] = nil //All car slots are empty at the beginning of the simulation
				}
			}
		}
	}

	for i := 0; i < data_definitions.N_streets_w_e; i++ { //Setup all crossing pointers, crossing lights and turn rates
		for j := 0; j < data_definitions.N_streets_n_s; j++ {
			crossings[i][j].turn_rate[0] = (*turn_rate_in)[0][i][j]
			crossings[i][j].turn_rate[1] = (*turn_rate_in)[1][i][j]
			crossings[i][j].Street_w_e = &streets_w_e[i]
			crossings[i][j].Street_n_s = &streets_n_s[j]
			crossings[i][j].Cross_light = &lights[((i+1)*(j+1) - 1)]
		}
	}

	if data_definitions.Thread_control == data_definitions.MULTI_THREAD_POOL {
		jobs = make(chan int, 2000)
		go worker(0, jobs)
		go worker(1, jobs)
	}
}

func update_remaining_slots(s_in *Street, i int, j int, dir int) {

	if s_in.street_slots[i][j] != nil {
		return //If there is a car on the actual car slot, proceed checking the other empty ones
	}

	if j != 0 && (j%slots_per_block) == 0 { //Check if it is on a crossing

		var temp_crossing Crossing
		if s_in.Direction == NORTH || s_in.Direction == SOUTH { //Check the crossing lights before moving traffic
			Get_crossing(&temp_crossing, (j/slots_per_block)-1, s_in.street_number) //Get the corresponding crossing which will bring the...
		} else {
			Get_crossing(&temp_crossing, s_in.street_number, (j/slots_per_block)-1) //...crossing street and both traffic lights
		}

		var temp_light_state light_data.Light_state = light_data.INVALID
		if s_in.Direction == NORTH || s_in.Direction == SOUTH {
			temp_light_state = temp_crossing.Cross_light.Light_n_s //Every crossing has two traffic lights
		} else {
			temp_light_state = temp_crossing.Cross_light.Light_w_e //So this "if" catches the correspondent light
		}

		if temp_light_state == light_data.RED {
			return //If the lights are red, don't move the car, proceed to the next car slot REQ: N
		}

		if s_in.street_slots[i][j] != nil || s_in.street_slots[i][j-dir] != nil { //REQ: K
			return //If the car slot after the crossing isn't empty, don't move the car, proceed to the next car slot
		} //Note that the crossing street isn't checked since its lights are red and the "crossing box" is always empty due to this condition

		var turn_rate int = 0
		if s_in.Direction == NORTH || s_in.Direction == SOUTH {
			turn_rate = temp_crossing.turn_rate[0] //Captures the turn rate base on street direction
		} else {
			turn_rate = temp_crossing.turn_rate[1]
		}

		if rand.Intn(100) < turn_rate { //Check if the car will turn to the other street REQ: J

			var temp_street *Street
			if s_in.Direction == NORTH || s_in.Direction == SOUTH {
				temp_street = temp_crossing.Street_w_e //Gets the crossing street
			} else {
				temp_street = temp_crossing.Street_n_s
			}

			var dir2 int
			if temp_street.Direction == NORTH || temp_street.Direction == WEST {
				dir2 = -1 //Gets the crossing street direction
			} else {
				dir2 = +1 //Note that the signal is inverted in relation to "dir"
			}

			x_slot := (s_in.street_number + 1) * slots_per_block //Calculates the slot of the crossing street to make the turn
			for k := 0; k < temp_street.Lane_count; k++ {        //Allows the turn to any lane, the first that is empty
				if temp_street.street_slots[k][x_slot] == nil && temp_street.street_slots[k][x_slot+dir2] == nil { //If the crossing is free of cars to make the turn
					temp_street.street_slots[k][x_slot+dir2] = s_in.street_slots[i][j+dir] //Change the car to the other street
					s_in.street_slots[i][j+dir] = nil                                      //Clear the car from the coming street
				}
			}
		}
	}
	s_in.street_slots[i][j] = s_in.street_slots[i][j+dir] //If it is not in a crossing, didn't turn and is not in a red light, move the car
	s_in.street_slots[i][j+dir] = nil
}

func update_cars(s_in *Street) {

	var dir int = 0
	if s_in.Direction == NORTH || s_in.Direction == WEST { //Depending on traffic direction, changes the way car slots are handled
		dir = +1
	} else {
		dir = -1
	}

	last_slot := (s_in.block_count * slots_per_block) - 1 //Calculates the number of the last slot of the street (southmost/eastmost point)

	for i := 0; i < data_definitions.Max_lanes; i++ {

		if i >= s_in.Lane_count {
			break //If the road doesn't have as many lanes, stop calculation
		}

		if dir > 0 { //If there is a car leaving the map, clears the slot and say the car is not on the map
			if s_in.street_slots[i][0] != nil {
				s_in.street_slots[i][0].On_map = false
			}
			s_in.street_slots[i][0] = s_in.street_slots[i][1] //Pulls the next car to the last slot, don't check crossing since there isn't any
			s_in.street_slots[i][1] = nil                     //Clear the slot that moved
		} else {
			if s_in.street_slots[i][last_slot] != nil {
				s_in.street_slots[i][last_slot].On_map = false
			}
			s_in.street_slots[i][last_slot] = s_in.street_slots[i][last_slot-1] //Pulls the next car to the last slot, don't check crossing since there isn't any
			s_in.street_slots[i][last_slot-1] = nil                             //Clear the slot that moved
		}

		if dir > 0 {
			for j := 1; j < last_slot; j++ {
				update_remaining_slots(s_in, i, j, dir)
			}
		} else {
			for j := (last_slot - 1); j > 0; j-- {
				update_remaining_slots(s_in, i, j, dir)
			}
		}
	}
}

func insert_car(s_in *Street, lane_in int, c_in *car_data.Car) int {

	if lane_in > s_in.Lane_count {
		return 0 //If the call is trying to place a car on a lane that isn't there, return
	}

	var input_slot int
	if s_in.Direction == NORTH || s_in.Direction == WEST {
		input_slot = (s_in.block_count * slots_per_block) - 1 //According to the mapping, places the car at the end of the road if it is headed north/west
	} else {
		input_slot = 0 //If not, put it at the beginning
	}

	if s_in.street_slots[lane_in][input_slot] != nil {
		return 0 //If the targeted place isn't empty, return false
	}

	s_in.street_slots[lane_in][input_slot] = c_in //Place the car at the start of the street

	return 1
}

func Insert_car_all_lanes_by_coordenate(id int, dir street_direction, c_in [data_definitions.Max_lanes]*car_data.Car) int {

	var temp_street *[data_definitions.N_streets_w_e]Street //Street pointer to handle north/south and west/east roads

	if dir == NORTH || dir == SOUTH { //Checks the street direction to get the right data set
		if id > (data_definitions.N_streets_n_s - 1) {
			id = (data_definitions.N_streets_n_s - 1) //Saturates the Id in between the actual data set
		}
		temp_street = &streets_n_s //Get the pointer right
	} else {
		if id > (data_definitions.N_streets_w_e - 1) {
			id = (data_definitions.N_streets_w_e - 1) //Saturates the Id in between the actual data set
		}
		temp_street = &streets_w_e //Get the pointer right
	}

	var inserts int
	for i := 0; i < temp_street[id].Lane_count; i++ { //Take a car from the input array and puts it in a street
		inserts += insert_car(&temp_street[id], i, c_in[i]) //Place the car in its lane
	}

	return inserts //Returns the amount of cars that were inserted
}

func move_cars_w_e() {

	for i := 0; i < data_definitions.N_streets_w_e; i++ {
		update_cars(&streets_w_e[i])
	}
}

func move_cars_n_s() {
	for i := 0; i < data_definitions.N_streets_n_s; i++ {
		update_cars(&streets_n_s[i])
	}
}

func worker(id int, jobs <-chan int) {
	for j := range jobs {
		if (j % 2) == 0 {
			for i := 0; i < data_definitions.N_streets_w_e; i++ {
				update_cars(&streets_w_e[i])
			}
		} else {
			for i := 0; i < data_definitions.N_streets_n_s; i++ {
				update_cars(&streets_n_s[i])

			}
		}
		ws.Done()
	}
}

var ws sync.WaitGroup
var job_counter int

func Move_cars_200ms() {
	if data_definitions.Thread_control == data_definitions.SINGLE_THREAD {
		for i := 0; i < data_definitions.N_streets_n_s; i++ {
			update_cars(&streets_n_s[i])
		}
		for i := 0; i < data_definitions.N_streets_w_e; i++ {
			update_cars(&streets_w_e[i])
		}
	} else if data_definitions.Thread_control == data_definitions.MULTI_THREAD {
		go move_cars_w_e()
		move_cars_n_s()
	} else if data_definitions.Thread_control == data_definitions.MULTI_THREAD_POOL {

		//North-south
		jobs <- job_counter
		ws.Add(1)
		job_counter++

		//West-east
		jobs <- job_counter
		job_counter++
		ws.Add(1)

		if data_definitions.Thread_sync == data_definitions.HARD_SYNC {
			ws.Wait()
		} else if data_definitions.Thread_sync == data_definitions.SOFT_SYNC {
			if (job_counter % 10) == 0 {
				ws.Wait()
			}
		} else if data_definitions.Thread_sync == data_definitions.NO_SYNC {
			//Don't wait
		}
	}
}

func Update_all_lights_1s() {

	for i := 0; i < light_data.N_lights; i++ {
		light_data.Update_light_1s(&lights[i])
	}
}

func Calculate_car_occupation() {

	total_cars_on_map = 0 //Clear the total car counter

	var temp_street *[data_definitions.N_streets_w_e]Street

	for h := 0; h < 2; h++ {
		if h == 0 {
			temp_street = &streets_w_e //One direction at a time
		} else {
			temp_street = &streets_n_s
		}

		for i := 0; i < data_definitions.N_streets_w_e; i++ {
			temp_street[i].Car_count = 0
			for j := 0; j < temp_street[i].block_count; j++ {

				counter := 0
				for k := 0; k < temp_street[i].Lane_count; k++ {
					for l := 0; l < slots_per_block; l++ {
						if temp_street[i].street_slots[k][l+j*slots_per_block] != nil {
							counter++
						}
					}
				}
				temp_street[i].Car_count += counter

				if counter == ((slots_per_block * temp_street[i].Lane_count) - temp_street[i].Lane_count) {
					counter += temp_street[i].Lane_count
				}

				temp := float32((counter * 100) / (slots_per_block * temp_street[i].Lane_count))
				if temp > 0.0000 { //After calculating the percentage of use, round up/
					temp += 0.5
				}
				if temp < 1.0 && temp > 0.0000 { //If it is below 1% and isn't zero, make it 1% so it is visible when plotting on the screen
					temp = 1.0
				}
				temp_street[i].Occupation[j] = int(temp) //Cast int to simplify what is shown to the user REQ:M

				if data_definitions.Control_strategy == data_definitions.CBCL_CONTROL {

					var temp_crossing Crossing

					if ((temp_street[i].Direction == WEST || temp_street[i].Direction == NORTH) && j > 0) || //Every street has one block more than crossing lights, so the first iteration is taken out of the controls
						((temp_street[i].Direction == SOUTH || temp_street[i].Direction == EAST) && j < (temp_street[i].block_count-1)) {

						var pos int
						if temp_street[i].Direction == WEST || temp_street[i].Direction == NORTH {
							pos = j - 1
						} else {
							pos = j
						}

						if temp_street[i].Direction == NORTH || //Check the crossing lights before moving traffic
							temp_street[i].Direction == SOUTH {
							Get_crossing(&temp_crossing, pos, temp_street[i].street_number)
						} else { //Get the corresponding crossing which will bring the...
							Get_crossing(&temp_crossing, temp_street[i].street_number, pos)
						} //...crossing street and both traffic lights
					}

					if temp_street[i].Occupation[j] > 60 { //REQ: #1 & #2 CBCL

						var direction light_data.Light_direction
						if temp_street[i].Direction == NORTH || temp_street[i].Direction == SOUTH {
							direction = light_data.NORTH_SOUTH
						} else {
							direction = light_data.WEST_EAST
						}
						light_data.Reduce_red_time(temp_crossing.Cross_light, direction)
					}
				}
			}
			total_cars_on_map += temp_street[i].Car_count //Accumulate the sum of cars on every street
		}
	}
}

func Get_street(s_in *Street, dir street_direction, street_id int) {
	if dir == WEST || dir == EAST {
		if street_id < data_definitions.N_streets_w_e {
			*s_in = streets_w_e[street_id]
		}
	}
	if dir == NORTH || dir == SOUTH {
		if street_id < data_definitions.N_streets_n_s {
			*s_in = streets_n_s[street_id]
		}
	}
}

func Get_crossing(c_in *Crossing, st_w_e int, st_n_s int) {
	if st_w_e < data_definitions.N_streets_w_e && st_n_s < data_definitions.N_streets_n_s {
		*c_in = crossings[st_w_e][st_n_s]
	}
}

func Get_total_car_count_on_map() int {
	return total_cars_on_map
}
