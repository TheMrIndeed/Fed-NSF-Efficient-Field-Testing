% Send a warning useing thing speak if the tunnel is expected to overheat

% Channel IDs to read data from:
temperatureChannelID = ;            %Update
weatherChannelID = ;                %Update
averageChannelID = ;                %Update

% API Keys:
temperatureReadAPIKey = '';         %Update
weatherReadAPIKey = '';             %Update
averageReadAPIKey = '';             %Update
averageWriteAPIKey = '';            %Update

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
