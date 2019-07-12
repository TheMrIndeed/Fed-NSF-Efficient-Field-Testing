% Send a warning useing thing speak if the tunnel is expected to overheat

% Number of responces to recieve
responces=40;

% URL of the page to read data from
url = append('https://api.openweathermap.org/data/2.5/forecast?lat=44.653426&lon=-93.003440&cnt=',int2str(responces),'&units=imperial&appid=82e84a17d4bc553ec6c7ecdcb8fe9bd5');
%% Scrape the webpage %%
Current_Data = webread(url);

% Channel IDs to read data from:
highChannelID = 819411;
averageChannelID = 819414;

% Write API Keys:
highReadAPIKey = '4N4S3MP0QPKJU4PF';
averageReadAPIKey = 'Y6T9XJR4M9HDVOHM';

%thingSpeakWrite(averageChannelID,'Fields',1,'Values',5,'WriteKey','V1M7876F78AIL0ZX');

highTemp=thingSpeakRead(highChannelID);
averageDifference=thingSpeakRead(averageChannelID);

message='Your tunnel is predicted to overheat at ';

j=0;

for i=1:responces
    if(j==0)
        if((Current_Data.list{i}.main.temp_max)>=(highTemp-averageDifference))
            message = append(message,Current_Data.list{i}.dt_txt);
            j=1;
        end
    elseif(j==1)
        if((Current_Data.list{i}.main.temp_max)>=(highTemp-averageDifference))
            message = append(message,' and ',Current_Data.list{i}.dt_txt);
            j=1;
        end
    end
end

messageUrl=append('https://maker.ifttt.com/trigger/highTemp/with/key/btnA3G9F_kjp7htOirUH5M/?value1=',message);

if(j==0)
    'No High Temperatures'
elseif(j==1)
    responce=webread(messageUrl)
end


%thingSpeakWrite(channelID,'Fields',3,dataTable,'WriteKey',writeAPIKey,)


%% Write Data %%
%%thingSpeakWrite(writeChannelID, analyzedData, 'WriteKey', writeAPIKey);
