// TrafficSimulator project main.go
package main

import (
	"TrafficSimulatorV2/data_definitions"
	"TrafficSimulatorV2/light_data"
	"TrafficSimulatorV2/street_data"
	"TrafficSimulatorV2/traffic_manager"
	"bufio"
	"fmt"
	"os"
	"os/exec"
	"time"
)

func main() {

	start := time.Now()

	clear_screen()
	resize_screen()
	traffic_manager.Init_traffic_manager()

	var tickControl <-chan time.Time
	var timeout <-chan time.Time

	tickScreen := time.Tick(1000 * time.Millisecond)
	if data_definitions.Processing_type == data_definitions.REAL_TIME_10X {
		tickControl = time.Tick(20 * time.Millisecond)
		timeout = time.After((time.Duration)(data_definitions.Sim_length/10) * time.Millisecond)
	} else {
		tickControl = time.Tick(200 * time.Millisecond)
		timeout = time.After((time.Duration)(data_definitions.Sim_length) * time.Millisecond)
	}

	if data_definitions.Processing_type == data_definitions.PROCESSOR_LIMITED {

		for counter := 0; counter <= data_definitions.Sim_length; counter += 200 {
			traffic_manager.Update_traffic_200ms()
		}
		street_data.Calculate_car_occupation()

	} else {

	Loop:
		for {
			select {
			case <-tickControl:
				traffic_manager.Update_traffic_200ms()
			case <-tickScreen:
				print_screen()
			case <-timeout:
				break Loop
			}
		}
	}

	print_screen()

	t := time.Now()
	elapsed := t.Sub(start)

	fmt.Printf("Elapsed time: %d ms", int(elapsed/time.Millisecond))
	reader := bufio.NewReader(os.Stdin)
	reader.ReadRune()
}

func resize_screen() {
	cmd := exec.Command("cmd", "/c", "mode", "140,55")
	cmd.Stdout = os.Stdout
	cmd.Run()
}

func clear_screen() {
	cmd := exec.Command("cmd", "/c", "cls")
	cmd.Stdout = os.Stdout
	cmd.Run()

}

func print_screen() {
	//Temporary data
	var temp_street street_data.Street

	//Always starts on a new canvas
	clear_screen()

	//Prints the direction of the north/south streets
	fmt.Printf(" ")
	for i := 0; i < data_definitions.N_streets_n_s; i++ {
		street_data.Get_street(&temp_street, street_data.NORTH, i)
		if temp_street.Direction == street_data.NORTH {
			fmt.Printf("         x")
		} else {
			fmt.Printf("         o")
		}
	}
	fmt.Printf("\n\n")

	//Prints the west/south and crossings information
	for i := 0; i <= data_definitions.N_streets_w_e; i++ {

		fmt.Printf("  ")
		for j := 0; j < data_definitions.N_streets_n_s; j++ {
			street_data.Get_street(&temp_street, street_data.NORTH, j)
			fmt.Printf("       %03d", temp_street.Occupation[i])
		}
		fmt.Printf("\n\n")

		if i >= data_definitions.N_streets_w_e {
			break
		}

		street_data.Get_street(&temp_street, street_data.WEST, i)
		if temp_street.Direction == street_data.WEST {
			fmt.Printf("x")
		} else {
			fmt.Printf("o")
		}

		//Prints the crossing information
		for j := 0; j <= data_definitions.N_streets_n_s; j++ {
			fmt.Printf("   %03d", temp_street.Occupation[j])
			if j != data_definitions.N_streets_n_s {
				var temp_crossing street_data.Crossing
				street_data.Get_crossing(&temp_crossing, i, j)

				if temp_crossing.Cross_light.Light_n_s != light_data.RED &&
					temp_crossing.Cross_light.Light_w_e == light_data.RED {
					if temp_crossing.Street_n_s.Direction == street_data.NORTH {
						fmt.Printf("   ^")
					} else {
						fmt.Printf("   v")
					}

				} else if temp_crossing.Cross_light.Light_n_s == light_data.RED &&
					temp_crossing.Cross_light.Light_w_e != light_data.RED {
					if temp_crossing.Street_w_e.Direction == street_data.WEST {
						fmt.Printf("   <")
					} else {
						fmt.Printf("   >")
					}
				} else {
					fmt.Printf("   +")
				}
			}
		}

		if temp_street.Direction == street_data.WEST {
			fmt.Printf("   o")
		} else {
			fmt.Printf("   x")
		}

		//Prints extra info from west/east
		fmt.Printf("  %1d", temp_street.Lane_count)
		fmt.Printf("  %04d", temp_street.Car_count)
		fmt.Printf("\n\n")
	}

	//Prints the direction of the north/south streets
	fmt.Printf(" ")
	for i := 0; i < data_definitions.N_streets_n_s; i++ {
		street_data.Get_street(&temp_street, street_data.NORTH, i)
		if temp_street.Direction == street_data.NORTH {
			fmt.Printf("         o")
		} else {
			fmt.Printf("         x")
		}

	}
	fmt.Printf("\n\n")

	//Prints extra info from north/south streets
	fmt.Printf(" ")
	for i := 0; i < data_definitions.N_streets_n_s; i++ {
		street_data.Get_street(&temp_street, street_data.NORTH, i)
		fmt.Printf("         %1d", temp_street.Lane_count)
	}
	fmt.Printf("\n\n")

	fmt.Printf("  ")
	for i := 0; i < data_definitions.N_streets_n_s; i++ {
		street_data.Get_street(&temp_street, street_data.NORTH, i)
		fmt.Printf("      %04d", temp_street.Car_count)
	}
	fmt.Printf("\n\n")

	//Prints some final statistics
	fmt.Printf("Total cars on map: %6d", street_data.Get_total_car_count_on_map())
	fmt.Printf("\n")
	fmt.Printf("Cars passed on map:%6d", traffic_manager.Get_cars_passed_on_map())
	fmt.Printf("\n")
}
