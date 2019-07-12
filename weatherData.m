% Get Data from Open Weather Map and write it to field 3

% URL of the page to read data from
url = 'https://api.openweathermap.org/data/2.5/weather?lat=44.653426&lon=-93.003440&units=imperial&appid=82e84a17d4bc553ec6c7ecdcb8fe9bd5';

% Channel ID to write data to:
weatherChannelID = ;                      %Update
% Write API Key:
weatherWriteAPIKey = '';                  %Update

%% Scrape the webpage %%
Current_Data = webread(url);

thingSpeakWrite(weatherChannelID,'Fields',1,'Values',Current_Data.main.temp_max,'WriteKey',weatherWriteAPIKey);
