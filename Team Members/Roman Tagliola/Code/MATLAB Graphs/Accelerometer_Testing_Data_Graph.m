clc 
clear
close all

% creating a table and a matrix of data based on a slightly modified
% version of the xcel data spreadsheet

accelerometer_data_table = readtable("Acceleration_Data.xlsx", ...
    "VariableNamingRule", "preserve");
accelerometer_data_matrix = table2array(accelerometer_data_table);

% separate columns of interest into apprppriately named matrices

brake_power = accelerometer_data_matrix(:, 3);
time_to_stop = accelerometer_data_matrix(:, 4);
acceleration = accelerometer_data_matrix(:, 5);
distance = accelerometer_data_matrix(:, 6);

%% manipulate the brake_power matrix to make a matrix of strings

k = length(brake_power);
brake_power_strings = strings(k,1);

for i = 1:length(brake_power)

    if brake_power(i) <= 0.4

        brake_power_strings(i) = "Low";

    elseif brake_power(i) <= 0.8

        brake_power_strings(i) = "Medium";

    else

        brake_power_strings(i) = "High";
    
    end

end

brake_power_categorical = categorical(brake_power_strings);
brake_power_categorical = reordercats(brake_power_categorical, ...
    ["Low", "Medium", "High"]);

%% create swarm chart to graph brake_power vs. time_to_stop

bp_vs_tts = figure;
swarmchart(brake_power_categorical, time_to_stop, 'o', 'filled')
hold on
title("Breaking Power vs. Time to Stop")
xlabel("Brake Power (%)")
ylabel("Time to Stop (seconds)")
proper_plot_format(bp_vs_tts)
hold off

saveas(gcf, 'bp_vs_tts.png')

%% create scatter plot to graph distance vs. acceleration

a_vs_d = figure;
scatter(distance, acceleration, 'o', 'filled')
hold on
title("Distance vs. Acceleration")
xlabel("Distance (meters)")
ylabel("Acceleration (meter/second^2)")
proper_plot_format(a_vs_d)
hold off

saveas(gcf, 'a_vs_d.png')

%% create swarm chart to graph brake_power vs. acceleration

bp_vs_a = figure;
swarmchart(brake_power_categorical, acceleration, 'o', 'filled')
hold on
title("Brake Power vs. Acceleration")
xlabel("Brake Power")
ylabel("Acceleration (m/s^2)")
proper_plot_format(bp_vs_a)
hold off

saveas(gcf, 'bp_vs_a.png')

%% create scatter plot to graph time_to_stop vs. distance

d_vs_tts = figure;
scatter(distance, time_to_stop, 'o', 'filled')
hold on
title("Distance vs. Time to Stop")
xlabel("Distance (meters)")
ylabel("Time to Stop (seconds)")
proper_plot_format(d_vs_tts)
legend("Raw Data", "Best Fit", 'Location', [0.5 0.855 0.1 0.05])
hold off

saveas(gcf, 'd_vs_tts.png')