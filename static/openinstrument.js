function StoreClient() {
}

StoreClient.prototype.SendRequest = function(path, request, response, success, error) {
  var serialized = new PROTO.Base64Stream();
  request.SerializeToStream(serialized);
  $.ajax({
    url: path,
    type: 'POST',
    data: serialized.getString().replace("\n", ""),
    dataType: 'text',
    success: function(text) {
      // we get all the data in one go, if we only got partial data, we could merge it with what we already got
      response.ParseFromStream(new PROTO.Base64Stream(text));
      if (response && response.success)
        success(response);
      else if (error)
        error(response);
    },
    error: function(jqXHR, textStatus, errorThrown) {
      response.success = false;
      response.errormessage = "Error in " + path + " request: " + textStatus + errorThrown;
      if (error)
        error(response);
      else
        console.log("Error in " + path + " request: " + errorThrown);
    },
  });
}

StoreClient.prototype.Add = function(request, success, error) {
  var response = new openinstrument.proto.AddResponse();
  this.SendRequest("/add", request, response, success, error);
  return response;
}

StoreClient.prototype.Get = function(request, success, error) {
  var response = new openinstrument.proto.GetResponse();
  this.SendRequest("/get", request, response, success, error);
  return response;
}

StoreClient.prototype.List = function(request, success, error) {
  var response = new openinstrument.proto.ListResponse();
  this.SendRequest("/list", request, response, success, error);
  return response;
}


function ParseDuration(str) {
  var matches = str.match(/^(\d+)y$/);
  if (matches) return parseInt(matches[1]) * 60 * 60 * 24 * 365;
  var matches = str.match(/^(\d+)w$/);
  if (matches) return parseInt(matches[1]) * 60 * 60 * 24 * 7;
  var matches = str.match(/^(\d+)d$/);
  if (matches) return parseInt(matches[1]) * 60 * 60 * 24;
  var matches = str.match(/^(\d+)h$/);
  if (matches) return parseInt(matches[1]) * 60 * 60;
  var matches = str.match(/^(\d+)m$/);
  if (matches) return parseInt(matches[1]) * 60;
  var matches = str.match(/^(\d+)s?$/);
  if (matches) return parseInt(matches[1]);
  return parseInt(str);
}

function NiceDuration(duration) {
  var bits = [];
  if (duration > 60 * 60 * 24 * 365) {
    return Math.floor(duration / (60 * 60 * 24 * 365)) + "y";
  } else if (duration >= 60 * 60 * 24 * 7) {
    return Math.floor(duration / (60 * 60 * 24 * 7)) + "w";
  } else if (duration >= 60 * 60 * 24) {
    return Math.floor(duration / (60 * 60 * 24)) + "d";
  } else if (duration >= 60 * 60) {
    return Math.floor(duration / (60 * 60)) + "h";
  } else if (duration >= 60) {
    return Math.floor(duration / (60)) + "m";
  } else if (duration > 0) {
    return Math.floor(duration) + "s";
  }
}

function Variable() {
  this.variable = ""
  this.labels = {}
}

Variable.FromString = function(str) {
  var matches = str.match(/^([^{]+){(.+)}/);
  if (!matches) return null;
  var variable = new Variable();
  variable.variable = matches[1];
  tmplabels = matches[2].split(/,/);
  for (var i in tmplabels) {
    var tmp = tmplabels[i].split(/=/, 2);
    variable.labels[tmp[0]] = tmp[1];
  }
  return variable;
}

