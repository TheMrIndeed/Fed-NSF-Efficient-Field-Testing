% Get Data from Open Weather Map and write it to field 3

% URL of the page to read data from
url = 'https://api.openweathermap.org/data/2.5/weather?lat=44.653426&lon=-93.003440&units=imperial&appid=82e84a17d4bc553ec6c7ecdcb8fe9bd5';

% TODO - Replace the [] with channel ID to write data to:
weatherChannelID = 819413;
% TODO - Enter the Write API Key between the '' below:
weatherWriteAPIKey = 'U7MD4J66M3NHQOHD';

%% Scrape the webpage %%
Current_Data = webread(url);

thingSpeakWrite(weatherChannelID,'Fields',1,'Values',Current_Data.main.temp_max,'WriteKey',weatherWriteAPIKey);
%Current_Data.main.temp_max
%datetime(Current_Data.dt,'ConvertFrom','posixtime')
