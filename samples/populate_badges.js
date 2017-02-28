function populate_row(branch) {
  var row = "";
  row += '<tr>';
  row += '<th>' + branch.name + '</th>';
  row += '<th><a href="http://travis-ci.org/guillaumeblanc/ozz-animation/branches" target="_blank"><img src="https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=' + branch.name + '" alt="' + branch.name + '"></a></th>';
  row += '<th><a href="http://travis-ci.org/guillaumeblanc/ozz-animation/branches" target="_blank"><img src="https://travis-ci.org/guillaumeblanc/ozz-animation.svg?branch=' + branch.name + '" alt="' + branch.name + '"></a></th>';
  row += '<th><a href="http://ci.appveyor.com/project/guillaumeblanc/ozz-animation" target="_blank"><img src="https://ci.appveyor.com/api/projects/status/github/guillaumeblanc/ozz-animation?branch=' + branch.name + '&svg=true"></a></th>';
  row += '</tr>';
  return row;
}

function populate_table(branches) {

    // Sort branches.
    branches.sort(function(a, b){
    	if(a.name == "master") return -1;
    	if(b.name == "master") return 1;
    	if(a.name == "develop") return -1;
    	if(b.name == "develop") return 1;
    	if(a.name.includes("hotfix") && !b.name.includes("hotfix")) return -1;
    	if(b.name.includes("hotfix") && !a.name.includes("hotfix")) return 1;
    	if(a.name.includes("release") && !b.name.includes("release")) return -1;
    	if(b.name.includes("release") && !a.name.includes("release")) return 1;
    	if(a.name.includes("feature") && !b.name.includes("feature")) return -1;
    	if(b.name.includes("feature") && !a.name.includes("feature")) return 1;
    	else return a.name.localeCompare(b.name);
    });

    // Iterate other branches and populate table.
    var out = "";
    for(var i = 0; i < branches.length; i++) {
    	var branch = branches[i];
      // Rejects web site branches.
    	if (branch.name.includes("gh-pages")) {
    		continue;
    	}
    	out += populate_row(branch);
    }

    // Insert into html table tag.
    document.getElementById("dashboard_table").innerHTML += out;
}

function create_request() {
  var xmlhttp = new XMLHttpRequest();
  xmlhttp.onreadystatechange = function() {
    if (this.readyState == XMLHttpRequest.DONE) {
      if (this.status == 201) {
        populate_table(JSON.parse(this.responseText));
      } else {
        document.getElementById("dashboard_table").innerHTML += "Failed to list branches." + this.status;
      }
    }
  };
  xmlhttp.open("GET", "https://api.github.com/repos/guillaumeblanc/ozz-animation/branches", true);
  xmlhttp.send();
}