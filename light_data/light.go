package light_data

import (
	"TrafficSimulatorV2/data_definitions"
	"math/rand"
)

type Light_state int

const (
	GREEN   = 1
	RED     = 2
	YELLOW  = 3
	INVALID = 4
)

type Light_pace int

const (
	NORMAL  = 0
	FASTER  = 1
	CHANGED = 2
)

type Light_direction int

const (
	NORTH_SOUTH = 0
	WEST_EAST   = 1
)

type Light struct {
	Timer     int
	Pace      Light_pace
	Light_n_s Light_state
	Light_w_e Light_state
}

const N_lights int = (data_definitions.N_streets_w_e * data_definitions.N_streets_n_s)
const light_timespan int = (data_definitions.Lights_red_time + data_definitions.Lights_green_time + data_definitions.Lights_yellow_time)
const light_timespan_reduce_red int = (data_definitions.Lights_red_time_reduce_red + data_definitions.Lights_green_time + data_definitions.Lights_yellow_time)
const light_timespan_no_change int = (data_definitions.Lights_red_time_no_change + data_definitions.Lights_green_time + data_definitions.Lights_yellow_time)

func Init_light(l_in *Light) {
	l_in.Pace = NORMAL
	l_in.Timer = rand.Intn(light_timespan)
	Update_light_1s(l_in)
}

func Update_light_1s(l_in *Light) {
	var temp_light_1 *Light_state
	var temp_light_2 *Light_state

	//Swaps the lights of the same crossing to avoid code repetition
	if l_in.Timer < (light_timespan / 2) {
		temp_light_1 = &l_in.Light_n_s //REQ: H
		temp_light_2 = &l_in.Light_w_e
	} else {
		temp_light_1 = &l_in.Light_w_e
		temp_light_2 = &l_in.Light_n_s
	}

	//Do a Green-Yellow-Red logic with dead time for a pair of crossing lights
	if (l_in.Timer % (light_timespan / 2)) < data_definitions.Lights_dead_time {
		*temp_light_1 = RED //REQ: I
	} else if (l_in.Timer % (light_timespan / 2)) < data_definitions.Lights_green_time {
		*temp_light_1 = GREEN
	} else {
		*temp_light_1 = YELLOW
	}

	*temp_light_2 = RED

	l_in.Timer = (l_in.Timer + 1) % light_timespan //Increase the light timer as a periodic function does

	//REQ: #3 & #4 CBCL
	if l_in.Pace == FASTER && //If there is a requirement to speed up this change of light
		l_in.Timer > data_definitions.Lights_dead_time && //If it is not on the dead time
		l_in.Timer < (light_timespan/2) { //If it is on the first half of the counting, i.e. speed up red for WEST and EAST crossings

		l_in.Timer += (data_definitions.Lights_red_time - data_definitions.Lights_red_time_reduced) //Speed up this cycle
		if l_in.Timer > (light_timespan / 2) {
			l_in.Timer = (light_timespan / 2) //Saturate in the semi-cycle
		}
		l_in.Pace = CHANGED //Clear the speed up flag
	}

	if l_in.Pace == FASTER && //If there is a requirement to speed up this change of light
		l_in.Timer > data_definitions.Lights_dead_time && //If it is not on the dead time
		l_in.Timer >= (light_timespan/2) { //If it is on the second half of the counting, i.e. speed up red for NORTH and SOUTH crossings

		l_in.Timer += (data_definitions.Lights_red_time - data_definitions.Lights_red_time_reduced) //Speed up this cycle
		if l_in.Timer > light_timespan {
			l_in.Timer = light_timespan //Saturate in the semi-cycle
		}
		l_in.Pace = CHANGED //Clear the speed up flag
	}

	if l_in.Pace == CHANGED &&
		(l_in.Timer == 0 || l_in.Timer == (light_timespan/2)) {
		l_in.Pace = NORMAL
	}
}

func Reduce_red_time(l_in *Light, dir Light_direction) {

	if dir == NORTH_SOUTH &&
		l_in.Light_n_s == RED &&
		l_in.Pace == NORMAL && //Light has to be red to take action -> REQ: #6 CBCL
		l_in.Timer > data_definitions.Lights_dead_time && //Take out the red light for the dead time to avoid buggy behavior
		l_in.Timer <= light_timespan_no_change { //Light can't be close from being green -> REQ: #5 CBCL

		if l_in.Timer <= light_timespan_reduce_red {
			l_in.Pace = FASTER //REQ: #3 CBCL
		} else {
			l_in.Timer = light_timespan - data_definitions.Lights_yellow_time //REQ: #4 CBCL
		}
	}

	if dir == WEST_EAST &&
		l_in.Light_w_e == RED && l_in.Pace == NORMAL && //Light has to be red to take action -> REQ: #6 CBCL_CONTROL
		l_in.Timer > data_definitions.Lights_dead_time && //Take out the red light for the dead time to avoid buggy behavior
		l_in.Timer <= data_definitions.Lights_red_time_no_change { //Light can't be close from being green -> REQ: #5 CBCL

		if l_in.Timer <= light_timespan_reduce_red {
			l_in.Pace = FASTER //REQ: #3 CBCL
		} else {
			l_in.Timer = data_definitions.Lights_green_time + data_definitions.Lights_yellow_time //REQ: #4 CBCL
		}
	}
}
