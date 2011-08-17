var graphdata;
var zoomstack = [];

function ResizeGraph() {
  $("#graph").width(window.innerWidth - 70);
  $("#graph").height(window.innerHeight - 100);
}

function round(val, decimals) {
  var mul = Math.pow(10, decimals);
  return Math.round(val * mul) / mul;
}

var graph_options = {
  series: {
    lines: { show: true },
    points: { show: false },
    shadowSize: 0
  },
  xaxis: { mode: "time" },
  yaxis: { min: 0, tickFormatter: function(v) {
    var val = v * 8;
    if (val > 1024 * 1024 * 1024)
      return round(val / 1024 / 1024 / 1024, 1) + "Gbps";
    if (val > 1024 * 1024)
      return round(val / 1024 / 1024, 1) + "Mbps";
    if (val > 1024)
      return round(val / 1024, 1) + "Kbps";
    return val + "bps";
  }},
  selection: { mode: "x" },
  grid: {
    backgroundColor: { colors: ["#fff", "#eee"] }
  },
  legend: { noColumns: 1 },
};

function ResetZoom() {
  delete graph_options["xaxis"]["min"];
  delete graph_options["xaxis"]["max"];
  $("#zoomout").attr("disabled", true);
}

function ShowDuration() {
  $("#nice_duration").val(NiceDuration(graph_duration));
}

var graph_variable = "/network/interface/stats/ifInOctets";
var graph_interface = "Dialer0";
var graph_host = "";
var value_type = 4;

var fetch_timeout;
function FetchData() {
  if (!graph_interface || !graph_host || !graph_variable) return;
  if (fetch_timeout)
    clearTimeout(fetch_timeout);
  var msg = new openinstrument.proto.GetRequest();
  msg.variable = graph_variable + "{hostname=" + graph_host + ",interface=" +
      graph_interface + "}";
  var now = new Date();
  msg.min_timestamp = PROTO.I64.fromNumber(now - graph_duration * 1000);

  var mutation = new openinstrument.proto.StreamMutation();
  mutation.sample_type = parseInt($("#value_type").val());
  msg.mutation = [mutation];

  var client = new StoreClient();
  client.Get(msg, function(response) {
    graphdata = [];
    for (var i = 0; i < response.stream.length; i++) {
      var stream = response.stream[i];
      var record = { "label": stream.variable }
      var data = [];
      for (var j = 0; j < stream.value.length; j++) {
        var value = stream.value[j];
        data.push([value.timestamp.toNumber(), value.value]);
      }
      record.data = data;
      graphdata.push(record);
    }
    $.plot($("#graph"), graphdata, graph_options);
    ResizeGraph();
    fetch_timeout = setTimeout(FetchData, 30000);
  }, function() {
    fetch_timeout = setTimeout(FetchData, 30000);
  });
}

function BuildSelectors() {
  var msg = new openinstrument.proto.ListRequest();
  msg.prefix = "/network/interface/stats/*{hostname=*,interface=*}";
  var client = new StoreClient();
  client.List(msg, function(response) {
    $("#variable option").remove();
    $("#interface option").remove();
    $("#host option").remove();
    var variables = {};
    var hosts = {};
    for (var i = 0; i < response.stream.length; i++) {
      var stream = response.stream[i];
      if (!stream)
        continue;
      var variable = Variable.FromString(stream.variable);
      variables[variable.variable] = 1;
      hosts[variable.labels["hostname"]] = 1;
    }
    graph_host = null;
    for (var i in variables) {
      if (!graph_variable)
        graph_variable = i;
      var selected = "";
      if (graph_variable == i)
        selected = "selected='1'"
      $("#variable").append("<option value='" + i + "' " + selected + ">" + i + "</option>");
    }
    for (var i in hosts) {
      var selected = "";
      if (!graph_host)
        graph_host = i;
      if (graph_host == i)
        selected = "selected='1'"
      $("#host").append("<option value='" + i + "' " + selected + ">" + i + "</option>");
    }
    BuildInterfaceList();
  });
}

function BuildInterfaceList() {
  var msg = new openinstrument.proto.ListRequest();
  msg.prefix = graph_variable + "{hostname=" + graph_host + ",interface=*}";

  var client = new StoreClient();
  client.List(msg, function(response) {
    $("#interface option").remove();
    for (var i = 0; i < response.stream.length; i++) {
      var stream = response.stream[i];
      var variable = Variable.FromString(stream.variable);
      var interface = variable.labels["interface"];
      if (!graph_interface)
        graph_interface = interface;
      var selected = "";
      if (graph_interface == interface)
        selected = "selected='1'"
      $("#interface").append("<option value='" + interface + "' " + selected + ">" + interface + "</option>");
    }
    FetchData();
  });
}

var graph_duration = 3600 * 12;

$(function () {
  BuildSelectors();
  var graph = $("#graph");
  $(window).resize(ResizeGraph);
  ShowDuration();
  $("#increase_duration").click(function(el) {
    graph_duration = ParseDuration($("#nice_duration").val()) * 2;
    ShowDuration();
    ResetZoom();
    FetchData();
  });
  $("#decrease_duration").click(function(el) {
    graph_duration = ParseDuration($("#nice_duration").val()) / 2;
    ShowDuration();
    ResetZoom();
    FetchData();
  });
  $("#nice_duration").change(function(el) {
    graph_duration = ParseDuration(el.val());
    ShowDuration();
    ResetZoom();
    FetchData();
  });
  $("#value_type").change(function(el) {
    FetchData();
  });
  $("#host").change(function(el) {
    graph_host = $(this).val();
    graph_interface = null;
    BuildInterfaceList();
  });
  $("#variable").change(function(el) {
    graph_variable = $(this).val();
    FetchData();
  });
  $("#interface").change(function(el) {
    graph_interface = $(this).val();
    FetchData();
  });
  graph.bind("plotselected", function(event, ranges) {
    if (graph_options["xaxis"]["min"]) {
      zoomstack.push([graph_options["xaxis"]["min"], graph_options["xaxis"]["max"]]);
    }
    graph_options["xaxis"]["min"] = ranges.xaxis.from;
    graph_options["xaxis"]["max"] = ranges.xaxis.to;
    $("#zoomout").removeAttr("disabled");
    $.plot(graph, graphdata, graph_options);
  });
  graph.bind("plotunselected", function(event) {});
  $("#zoomout").click(function(event) {
    if (zoomstack.length) {
      var el = zoomstack.pop();
      graph_options["xaxis"]["min"] = el[0];
      graph_options["xaxis"]["max"] = el[1];
      $("#zoomout").removeAttr("disabled");
    } else {
      ResetZoom();
    }
    $.plot($("#graph"), graphdata, graph_options);
  });

  FetchData();
  ResizeGraph();
});

