function standard_plot_format(figure_handle)

figure(figure_handle)

% create medium-pink-to-red colormap

medium_pink_to_red = [linspace(1, 0.8, 256)', linspace(0.5, 0, 256)',... 
    linspace(0.5, 0, 256)'];

% look for each axis in the figure

ax = findall(figure_handle, 'Type', 'axis');

%loop through each axis in case of subplot

for i = 1:length(ax)

    chart_object = findall(ax, 'Type', 'scatter');

    % color all the data point according to the colormap
    % create colorbar

    y_data = chart_object.YData;
    chart_object.CData = y_data;
    colormap(ax(i), medium_pink_to_red);
    colorbar(ax(i));

    % format colorbar

    cb = colorbar(ax(i));
    cb.FontName = 'Times New Roman';
    cb.FontSize = 14;

end

% set title and axis label text to specific font and size

% loop through each axis in case of subplot

for j = 1:length(ax)

    chart_object = findall(ax, 'Type', 'scatter');

    % change all font of all text and set a base text size

    ax(j).FontName = 'Times New Roman';
    ax(j).FontSize = 12;

    % set font settings for title

    ax(j).Title.FontSize = 18;
    ax(j).Title.FontWeight = 'bold';

    % set font settings for axes

    ax(j).XLabel.FontSize = 14;
    ax(j).YLabel.FontSize = 14;
    ax(j).XLabel.FontWeight = 'normal';
    ax(j).YLabel.FontWeight = 'normal';

end

% set grid on, configure a y-axis limit, and create and format the 
% best fit line

% loop through each axis in case of subplot

for r = 1:length(ax)

    chart_object = findall(ax, 'Type', 'scatter');

    grid(ax(r), 'on');

    %set y-axis limit

    y_max = ceil(max(y_data));  % ceil() function always rounds up a value
    ylim(ax(r), [0, y_max]);

    % create the line of best fit

    x_data = chart_object.XData;

    % covert to floats

    x_data = double(x_data);
    y_data = double(y_data);

    polynomial = polyfit(x_data, y_data, 2); % creates a quadratic best fit

    % make some extra points to smooth the curve

    x_smooth = linspace(min(x_data), max(x_data), 100);
    y_smooth = polyval(polynomial, x_smooth);

    % plot the best fit curve

    hold(ax(r), 'on');
    plot(x_smooth, y_smooth, 'Color', '[1, 0.65, 0.65]', 'LineWidth',1 );
    hold(ax(r), 'off');

    % add legend and the best fit equation

    legend(ax(r), 'show', 'Raw Data', 'Best Fit Curve');

    c_1 = polynomial(1);
    c_2 = polynomial(2);
    c_3 = polynomial(3);

    best_fit_equation = sprintf("Line of Best Curve: y = %.2fx^2 + " + ...
        "%.2fx + %.2x", c_1, c_2, c_3);

    text(ax(r), 1.4, 9.45, best_fit_equation, 'FontSize', 12, ...
        'FontName', 'Times New Roman', 'BackgroundColor', 'black', ...
        'Edgecolor', 'white', 'Margin', 2);

end

