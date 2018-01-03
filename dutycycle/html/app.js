/**
 * Created by joel on 2017-12-30.
 */

var dcApp = angular.module('dcApp', []);

dcApp.controller('DutyCycleController', function DutyCycleController($scope, $timeout) {
    // Length of the window in which we do the on/off switching
    // Longer window will increase the fidelity but shorter window length means faster distribution between phases.
    $scope.window_length = 500;

    var countdown_to_start = 2;
    var loop_counter = 0;

    // How many percent power we want to put out
    $scope.duty_cycle_percentage = 97;

    // The minimum amount of time we want each state to be, ie, either On or Off for At least this amout of time
    $scope.min_flicker_length = 100;

    // Holds states for each of the phases.
    $scope.elements = [0, 0, 0];

    var element_order = [0, 1, 2];

    // Get length of window (but not too short)
    function getWindowLength(){
        var window_length = parseInt($scope.window_length);

        if(window_length < ($scope.min_flicker_length * 3)){
            return $scope.min_flicker_length * 3;
        }

        return window_length;
    }

    // Current milliseconds.
    function millis(){
        var d = new Date();
        return d.getTime();
    }

    // Get percentage as int
    function getDutyCyclePercentage(){
        return parseInt($scope.duty_cycle_percentage, 10);
    }

    /*
    * Get how many milliseconds each of the elements should be on
    * */
    function getOnTimeDistribution(){
        var percentage = getDutyCyclePercentage();
        var number_of_outputs = _.size($scope.elements);
        // How much time we should distribute between all of the elements
        var milliseconds_of_on_time = (parseInt($scope.window_length * (percentage / 100), 10)) * number_of_outputs;
        var onTime_distribution = [0,0,0];
        var max_onTime_possible = $scope.window_length * number_of_outputs;
        var outputs_to_engage = $scope.elements;
        var accumulated_on_time = 0;
        var milliseconds_to_distribute = milliseconds_of_on_time;

        // If the gap between the number of milliseconds we should distribute and the total is Less than the min
        // flicker length, change the milliseconds to distribute to accomodate for the min flicker length
        if(milliseconds_to_distribute < max_onTime_possible){
            if(milliseconds_to_distribute > (max_onTime_possible - $scope.min_flicker_length)){
                milliseconds_to_distribute = max_onTime_possible - $scope.min_flicker_length;
            }
        }

        // Which elements we should engage with.
        outputs_to_engage = _.slice(outputs_to_engage, 0, _.floor(milliseconds_to_distribute / $scope.min_flicker_length));

        // Go through all elements we should engage with and set the ammount of on-time they should have.
        _.each(outputs_to_engage, function(state, i){
            var on_time = _.floor(milliseconds_to_distribute / (_.size(outputs_to_engage) - i));

            if(on_time > ($scope.window_length - $scope.min_flicker_length)){
                on_time = parseInt($scope.window_length);
            }

            accumulated_on_time = accumulated_on_time + on_time;
            milliseconds_to_distribute = milliseconds_to_distribute - on_time;

            _.set(
                onTime_distribution,
                _.get(element_order, i),
                on_time
            )
        });

        return onTime_distribution;
    }

    function getStateForOutput(frame, onTime, output_number){
        var start_frame = _.floor(($scope.window_length / 3) * output_number);
        var state;

        if(onTime < 1){
            return 0;
        }

        var end_frame = start_frame + onTime;

        if(end_frame > $scope.window_length){
            end_frame = end_frame - $scope.window_length;
        }

        // If the Start frame is After the End frame, ie.
        // If this output wraps around the end of the window
        if(start_frame == end_frame){
            // Full patte
            state = 1;
        }else{
            if(start_frame > end_frame){
                state = 1;
                if(frame > end_frame && frame < start_frame){
                    state = 0;
                }
            }else{
                state = 0;
                if(frame > start_frame && frame < end_frame){
                    state = 1;
                }
            }
        }

        return state;

    }

    function loop(){
        var onTime_distribution = getOnTimeDistribution();
        var frame = millis() % $scope.window_length;
        var outputs = [0, 0, 0];

        if($scope.frame > frame){
            loop_counter += 1;
            console.log(loop_counter);
            if(loop_counter % 10 == 0){
                var first_element_number = element_order[0];
                element_order[0] = element_order[1];
                element_order[1] = element_order[2];
                element_order[2] = first_element_number;
                console.log(element_order);
            }
        }

        // through all the elements
        // check for what frames they should have what states
        _.each($scope.elements, function(output, i){
            var state = getStateForOutput(frame, onTime_distribution[i], i);

            _.set(
                outputs,
                i,
                state
            )
        });

        $scope.elements = outputs;
        $scope.frame = frame;
        //$timeout(loop, _.random(400,500));

        $timeout(loop, _.random(1,10));
    }

    $timeout(loop, 1);
});