// Fetch the data to simulate curl -X POST -H 'Content-Type: application/json' --data-raw '{"name":"mcharrad","start_date":"2023-07-29","end_date":"2023-08-24"}' http://logtime-med.1337.ma/api/get_log

const data = {
    name: 'mcharrad',
    start_date: '2023-07-29',
    end_date: '2023-08-24',
};

GM_xmlhttpRequest({
    method: 'POST',
    url: 'http://logtime-med.1337.ma/api/get_log',
    headers: {
        'Content-Type': 'application/json',
    },
    data: JSON.stringify(data),
    onload: function(response) {
        if (response.status !== 200) {
            console.error(`HTTP error! Status: ${response.status}`);
            return;
        }
        
        const responseData = JSON.parse(response.responseText);
        console.log(responseData);
    },
    onerror: function(error) {
        console.error(error);
    },
});

