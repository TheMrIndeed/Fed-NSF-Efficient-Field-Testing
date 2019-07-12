% Send a warning useing thing speak if the tunnel is expected to overheat

% Channel IDs to read data from:
temperatureChannelID = 808957;
weatherChannelID = 819413;
averageChannelID = 819414;

% API Keys:
temperatureReadAPIKey = 'UQEJTA2SKM3B68L6';
weatherReadAPIKey = 'X8K484COIMNY2HDR';
averageReadAPIKey = 'Y6T9XJR4M9HDVOHM';
averageWriteAPIKey = 'V1M7876F78AIL0ZX';

%thingSpeakWrite(averageChannelID,'Fields',1,'Values',5,'WriteKey','V1M7876F78AIL0ZX');

temperature = thingSpeakRead(temperatureChannelID,'NumDays',1,'OutputFormat','table');
weather = thingSpeakRead(weatherChannelID,'NumDays',1,'OutputFormat','table');
averageDifference = thingSpeakRead(averageChannelID);

temperature = rmmissing(temperature);
weather = rmmissing(weather);

weather = table2cell(weather);
temperature = table2cell(temperature);
[r1,c1]=size(weather)
[r2,c2]=size(temperature)


for i = 1:r1
    for j = 1:r2
        if(between(weather{i,1},temperature{i,1},'Minutes')<=5)
            averageDifference=((averageDifference*100+(temperature{2,i}-weather{2,i}))/101);
            break;
        end
    end
end

averageDifference

%% Write Data %%
thingSpeakWrite(averageChannelID,'Fields',1,'Values',averageDifference,'WriteKey',averageWriteAPIKey);
