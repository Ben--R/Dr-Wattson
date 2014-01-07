var devices = document.getElementById("devices");

console.log(devices.innerHTML);

function appendDevice(name)
{
	devices.innerHTML += "<div class=\"device\"><h3>" + name + "</h3></div>";
}

appendDevice("Lorem");
appendDevice("Ipsum");
appendDevice("Dolor");
appendDevice("Sit");
appendDevice("Amet");

for (var i=0;i<100;i++)
{
	appendDevice(i.toString());
}
