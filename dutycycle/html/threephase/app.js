/**
 * Created by joel on 2017-12-30.
 */

var dcApp = angular.module('dcApp', []);

dcApp.controller('DutyCycleController', function DutyCycleController($scope, $timeout) {
    // Length of the window in which we do the on/off switching
    // Longer window will increase the fidelity but shorter window length means faster distribution between phases.
    $scope.window_length = 1000;

    var countdown_to_start = 2;
    var loop_counter = 0;
    var SHIFT_RELAYS_INTERVAL = 1;

    // How many percent power we want to put out
    $scope.duty_cycle_percentage = 20;

    // The minimum amount of time we want each state to be, ie, either On or Off for At least this amout of time
    $scope.min_flicker_length = 100;

    // Holds states for each of the phases.
    $scope.relays = [0, 0, 0];

    var relay_order = [1, 2, 3];

    function randomizeRelayOrder(){
        relay_order = _.shuffle(relay_order);
    }

    randomizeRelayOrder();

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
    * Get how many milliseconds each of the relays should be on
    * */
    function getOnTimeDistribution(){
        var percentage = getDutyCyclePercentage();
        var min_flicker_length = $scope.min_flicker_length;
        // How much time we should distribute between all of the relays
        var factor = (percentage / 100);
        var window_length = getWindowLength();
        var onTime_distribution = [0, 0, 0];
        var relays_to_engage;
        var onTime;

        // Which relays we should engage with and for how long
        if(percentage <= 33){
            // If we are at or lower than thirty percent, only use L1 and L2,
            // If for instance we are at 30 percent, these should be on 100% of the time
            relays_to_engage = _.take(relay_order, 2);
            // If we are at 33, it should be 100%
            onTime = window_length * (factor * 3);
        }else{
            relays_to_engage = _.take(relay_order, 3);
            onTime = window_length * factor;
        }

        // Go through all relays we should engage with and set the ammount of on-time they should have.
        _.each(relays_to_engage, function(relay_number, i){
            if(onTime > (window_length - min_flicker_length)){
                onTime = window_length;
            }

            _.set(
                onTime_distribution,
                relay_number - 1,
                onTime
            )
        });

        return onTime_distribution;
    }

    function loop(){
        var onTime_distribution;
        var frame = millis() % $scope.window_length;
        var outputs = [0, 0, 0];

        if($scope.frame > frame){
            loop_counter += 1;
            console.log(loop_counter);
            if(loop_counter % SHIFT_RELAYS_INTERVAL == 0){
                var first_element_number = relay_order[0];
                relay_order[0] = relay_order[1];
                relay_order[1] = relay_order[2];
                relay_order[2] = first_element_number;
                console.log(relay_order);
            }
        }

        onTime_distribution =  getOnTimeDistribution();

        // through all the relays
        // check for what frames they should have what states
        _.each($scope.relays, function(output, i){
            _.set(
                outputs,
                i,
                frame < onTime_distribution[i]
            )
        });

        $scope.relays = outputs;
        $scope.frame = frame;
        //$timeout(loop, _.random(400,500));

        $timeout(loop, _.random(1,10));
    }

    $timeout(loop, 1);
});