package traffic_manager

import (
	"TrafficSimulatorV2/car_data"
	"TrafficSimulatorV2/data_definitions"
	"TrafficSimulatorV2/street_data"
)

const car_buffer_size int = 500000

var car_buffer [car_buffer_size]car_data.Car //Big buffer to avoid having to search for unused cars before re-inserting, done this way for simplicity's sake
var last_used_car_id int
var traffic_tick_1s int
var traffic_flow_tick int
var traffic_flow_counter int

func Init_traffic_manager() {

	traffic_tick_1s = 0      //Starts the loops divider with a zero valued number
	traffic_flow_tick = 0    //Starts the flow control tick counter with zero
	traffic_flow_counter = 0 //Starts the 200ms counter with zero

	if data_definitions.Test_scenario == data_definitions.SCENARIO_1 {
		street_data.Init_streets(2, data_definitions.N_streets_w_e,
			&data_definitions.Number_of_lanes_input_sc1,
			&data_definitions.Turn_rate_input_sc1)
	} else {
		street_data.Init_streets(2, data_definitions.N_streets_w_e,
			&data_definitions.Number_of_lanes_input_sc2,
			&data_definitions.Turn_rate_input_sc2)
	}

	for i := 0; i < car_buffer_size; i++ { //Starts all car with a unique ID and outside all streets
		car_buffer[i].Id = i + 1
		car_buffer[i].On_map = false
	}
	last_used_car_id = 0 //None of the cars are in use at the start of the program
}

func insert_cars_200ms() {

	var flag_tick int = 0

	if data_definitions.Traffic_intensity <= 5 { //Linear car rate input - constant flow

		traffic_flow_tick++
		if traffic_flow_tick%(50/int(data_definitions.Traffic_intensity)) == 0 {
			flag_tick = 1
		}
	} else if data_definitions.Traffic_intensity == data_definitions.CAR_RATE_LINEAR_GROWTH {

		traffic_flow_counter++
		traffic_flow_tick++

		temp_calc := 1 + int(traffic_flow_counter/2000)

		if (traffic_flow_tick % (50 / temp_calc)) == 0 {
			flag_tick = 1
		}

	} else if data_definitions.Traffic_intensity == data_definitions.CAR_RATE_POISSON {

		traffic_flow_counter++
		traffic_flow_tick++

		var temp_calc int
		if traffic_flow_counter <= 2000 {
			temp_calc = 1
		} else if traffic_flow_counter <= 4000 {
			temp_calc = 5
		} else if traffic_flow_counter <= 6000 {
			temp_calc = 4
		} else if traffic_flow_counter <= 8000 {
			temp_calc = 2
		} else {
			temp_calc = 1
		}

		if (traffic_flow_tick % (50 / temp_calc)) == 0 {
			flag_tick = 1
		}

	} else {

		traffic_flow_tick++
		if (traffic_flow_tick % 10) == 0 {
			flag_tick = 1
		}
	}

	if flag_tick == 1 {
		flag_tick = 0

		traffic_flow_tick = 0

		var sub_car_buffer [4]*car_data.Car
		for i := 0; i < 4; i++ {
			sub_car_buffer[i] = &car_buffer[last_used_car_id+i]
		}

		for i := 0; i < data_definitions.N_streets_w_e; i++ {
			last_used_car_id += street_data.Insert_car_all_lanes_by_coordenate(i, street_data.WEST, sub_car_buffer)
		}
		for i := 0; i < data_definitions.N_streets_n_s; i++ {
			last_used_car_id += street_data.Insert_car_all_lanes_by_coordenate(i, street_data.NORTH, sub_car_buffer)
		}
	}
}

func Update_traffic_200ms() {

	street_data.Move_cars_200ms() //REQ: G 4m every 200ms-> 20m/s
	insert_cars_200ms()

	traffic_tick_1s = (traffic_tick_1s + 1) % 5
	if traffic_tick_1s == 0 {
		if data_definitions.Processing_type == data_definitions.PROCESSOR_LIMITED &&
			data_definitions.Control_strategy == data_definitions.CBCL_CONTROL {
			street_data.Calculate_car_occupation()
		}
		street_data.Update_all_lights_1s()
	}

	if data_definitions.Processing_type != data_definitions.PROCESSOR_LIMITED {
		street_data.Calculate_car_occupation()
	}
}

func Get_cars_passed_on_map() int {
	return last_used_car_id
}
