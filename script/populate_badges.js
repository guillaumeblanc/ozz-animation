var myTableBadges = (function () {

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

  function populate_table(_table_id, branches) {
    // Sort branches order.
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

    // Iterate through all branches to fill each table row.
    var out = "";
    for(var i = 0; i < branches.length; i++) {
      var branch = branches[i];
      // Rejects web site branches.
      if (branch.name.includes("gh-pages")) {
      	continue;
      }
      out += populate_row(branch);
    }
    // Fills the table.
    document.getElementById(_table_id).innerHTML += out;
  }

  // Creates an http request, fills the table if request succeeds.
  function create_request(_table_id, _url) {
    var xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function() {
      if (this.readyState == XMLHttpRequest.DONE) {
        if (this.status == 200) {
          populate_table(_table_id, JSON.parse(this.responseText));
        } else {
          document.getElementById(_table_id).innerHTML += "Failed to list branches: " + this.status;
        }
      }
    };
    xmlhttp.open("GET", _url, true);
    xmlhttp.send();
  }

  return {
    populate: create_request
  };
})();
