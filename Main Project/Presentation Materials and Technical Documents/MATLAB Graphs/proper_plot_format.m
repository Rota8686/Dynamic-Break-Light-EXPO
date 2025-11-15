% first name the function and the input argument

function proper_plot_format(figure_handle)

figure(figure_handle)

theme('light');

%% set a color map to color the data points

% create a medium-pink-to-red colormap

white_to_red = [linspace(1, 0.8, 256)', linspace(0.5, 0, 256)', ...
    linspace(0.5, 0, 256)'];

% look for each axis in the figure 

ax_color = findall(figure_handle, 'Type', 'axes');

%loop through each axis in case of subplot

for i = 1:length(ax_color)

    chart_object_1 = findall(ax_color(i), 'Type', 'scatter');

    % color all the points according to the white-to-red colormap and
    % create colorbar

    y_data_1 = chart_object_1.YData;
    chart_object_1.CData = y_data_1;
    colormap(ax_color(i), white_to_red);
    
end


%% set title and axis label text to specific font and size

% look for each axis in the figure

ax_text = findall(figure_handle, 'Type', 'axes');

% loop through each axis in case of subplot

for j = 1:length(ax_text)

    % change font of all text and set size for axes text

    ax_text(j).FontName = 'Times New Roman';
    ax_text(j).FontSize = 12;

    % set font settings for title

    ax_text(j).Title.FontSize = 18;
    ax_text(j).Title.FontWeight = 'bold';

    % set font settings for axes

    ax_text(j).XLabel.FontSize = 15;
    ax_text(j).YLabel.FontSize = 15;
    ax_text(j).XLabel.FontWeight = 'normal';
    ax_text(j).YLabel.FontWeight = 'normal';

end

%% set grid on, configure a y-axis limit, and create and format
%  a line of best fit

% look for each axis in the figure

ax_grid = findall(figure_handle, 'Type', 'axes');

%loop through each axis, in case of sublot

for r = 1:length(ax_grid)

    grid(ax_grid(r), 'on');

    %set the y-axis limit

    chart_object_2 = findall(ax_grid(r), 'Type', 'scatter');

    y_data_2 = chart_object_2.YData;
    y_max = ceil(max(y_data_2));  % ceil function always rounds up a value
    ylim(ax_grid(r), [0, y_max]);

    % create the line of best fit

    x_data_2 = chart_object_2.XData;
    % convert to floats
    x_data_2 = double(x_data_2);
    y_data_2 = double(y_data_2);
    polynomial = polyfit(x_data_2, y_data_2, 2);  % this line creates quadratic best fit
    % make some extra point to smooth the curve
    x_smooth = linspace(min(x_data_2), max(x_data_2), 100);
    y_smooth = polyval(polynomial, x_smooth);
    % plot the curve
    hold(ax_grid(r), 'on');
    plot(ax_grid(r), x_smooth, y_smooth, "Color", '[1.0, 0.35, 0.35]', "LineWidth", 1);
    hold(ax_grid(r), 'off');

    % also add a legend and the best fit equation

    legend(ax_grid(r), "show", "Raw Data", "Best Fit Line");

    c_1 = polynomial(1);
    c_2 = polynomial(2);
    c_3 = polynomial(3);

    best_fit_equation = sprintf("Best Fit Curve: y = %.2fx^2 + " + ...
        "%.2fx + %.2x", c_1, c_2, c_3);

    text(ax_grid(r), 0, 0, best_fit_equation, 'FontSize', 12, ...
        'FontName', 'Times New Roman', 'BackgroundColor', 'white', ...
        'Edgecolor', 'black', 'Margin', 2);

end