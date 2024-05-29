const seriesConfigs = [
  {
    type: "line",
    name: "Обороты (об/мин)",
    value_name: "rpm",
    floor: 0,
    min: 0,
    lineWidth: 1,
    opposite: true,
  },
  {
    type: "line",
    name: "Нагрузка (г)",
    value_name: "loadCell",
    lineWidth: 1,
    min: 0,
    opposite: true,
    // startOnTick: false,
  },
  {
    name: "Заданное значение (%)",
    value_name: "setValue_percent",
    floor: 0,
    min: 0,
    max: 100,
    // visible: false,
    lineWidth: 1,
    opposite: true,
  },
  {
    name: "Мощность (Вт)",
    value_name: "power",
    floor: 0,
    min: 0,
    // max: 30,
    // visible: false,
    lineWidth: 1,
    opposite: true,
  },
  {
    name: "Напряжение (В)",
    value_name: "batVoltage_v",
    floor: 0,
    min: 0,
    // max: 30,
    visible: false,
    lineWidth: 1,
    opposite: false,
  },

].map((v, i) => {
  return { ...v, yAxis: i };
});

const loadCellChart = Highcharts.stockChart("chartContainer", {
  chart: {
    type: "line",
    zooming: { type: "x" },
    plotOptions: {
      series: {
        animation: false,
        marker: {
          enabled: false,
        },
      },
    },

    xAxis: {
      type: "datetime",
    },
  },

  yAxis: seriesConfigs.map((v) => {
    return {
      opposite: v.opposite,
      floor: v.floor,
      min: v.min,
      max: v.max,
      startOnTick: v.startOnTick,
      title: {
        text: v.name,
      },
    };
  }),
  rangeSelector: {
    buttons: [
      // {
      //   type: "millisecond",
      //   count: 60 * 1 * 1000,
      //   text: "60",
      //   title: "Посмотреть 1 минуту",
      // },
      {
        type: "millisecond",
        count: 120 * 1 * 1000,
        text: "120",
        title: "Посмотреть 2 минуты",
      },
      {
        type: "millisecond",
        count: 300 * 1 * 1000,
        text: "300",
        title: "Посмотреть 5 минут",
      },
      {
        type: "millisecond",
        count: 600 * 1 * 1000,
        text: "600",
        title: "Посмотреть 10 минут",
      },
      {
        type: "all",
        text: "Всё",
        title: "Посмотреть всё",
      },
    ],
    inputEnabled: false,
    selected: 0,
  },
  // series: seriesConfigs.map((v) => v),

  title: {
    text: "Мотор-тестер",
  },
  tooltip: {
    split: false,
    shared: true,
    headerFormat: "<span>{point.x/1000}</span><br/>",
    valueDecimals: 2,
    style: {
      fontSize: "20px",
    },
  },

  exporting: {
    enabled: false,
  },
  legend: {
    enabled: true,
    style: {
      fontSize: "20px",
    },
    itemStyle: {
      fontSize: "20px",
    },
  },
});

{
  for (var seriesConfig of seriesConfigs) {
    loadCellChart.addSeries({ ...seriesConfig });
    // loadCellChart.addSeries({ ...seriesConfigs[1], data: [1, 2] });
  }
}
var gateway = `ws://${
  window.location.protocol == "http"
    ? window.location.hostname
    : "zmotor_tester.local"
}/liveData`;
var websocket;
window.addEventListener("load", onLoad);
function initWebSocket() {
  console.log("Trying to open a WebSocket connection...");
  websocket = new WebSocket(gateway);
  websocket.onopen = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}
function onOpen(event) {
  console.log("Connection opened");
}
function onClose(event) {
  console.log("Connection closed");
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  let wsData = JSON.parse(event.data);

  for (let seriesConfig of seriesConfigs) {
    let series = loadCellChart.series.find((v) => v.name == seriesConfig.name);
    if (series) {
      series.addPoint(
        [wsData.mtime, wsData[seriesConfig.value_name]],
        false,
        false,
        false
      );
    }
  }
  loadCellChart.redraw(false);
}

function onLoad(event) {
  initWebSocket();
}
