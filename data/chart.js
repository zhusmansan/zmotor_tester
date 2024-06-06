function updateLegendLabel(chrt) {
  // var chrt = !this.chart ? this : this.chart;
  chrt.update({});
}

const seriesConfigs = [
  {
    type: "line",
    name: "Обороты",
    unit: "(об/мин)",
    value_name: "rpm",
    min: 0,
    softMax: 1000,
    opposite: true,
  },
  {
    type: "line",
    name: "Нагрузка",
    unit: "(г)",
    value_name: "loadCell",
    // lineWidth: 1,
    softMin: 0,
    softMax: 300,
    opposite: true,
    endOnTick: false,
    // startOnTick: false,
  },
  {
    name: "Заданное значение",
    unit: "(%)",
    value_name: "setValue_percent",
    floor: 0,
    min: 0,
    max: 100,
    // visible: false,
    // lineWidth: 1,
    opposite: true,
  },
  {
    name: "Мощность",
    unit: "(Вт)",
    value_name: "power",
    floor: 0,
    min: 0,
    softMax: 5,
    // visible: false,
    // lineWidth: 1,
    opposite: true,
  },
  {
    name: "Напряжение",
    unit: "(В)",
    value_name: "batVoltage_v",
    floor: 0,
    min: 0,
    // max: 30,
    visible: false,
    // lineWidth: 1,
    opposite: false,
  },
  {
    name: "Ток",
    unit: "(мА)",
    value_name: "batCurrent_ma",
    floor: 0,
    min: 0,
    // max: 30,
    visible: false,
    // lineWidth: 1,
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
        dataLabels: {
          enabled: true,
        },
      },
    },

    xAxis: {
      type: "datetime",
    },
  },

  yAxis: seriesConfigs.map((v) => {
    return {
      ...v,
      visible: true,
      startOnTick: v.startOnTick,
      title: {
        text: v.name,
      },
    };
  }),
  rangeSelector: {
    buttons: [
      {
        type: "second",
        count: 30,
        text: "30",
        title: "Посмотреть 30 секунд",
      },
      {
        type: "second",
        count: 60,
        text: "60",
        title: "Посмотреть 60 секунд",
      },
      {
        type: "second",
        count: 90,
        text: "90",
        title: "Посмотреть 90 секунд",
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

  tooltip: {
    split: false,
    shared: true,
    headerFormat: "<b>{point.x:%H:%M:%S.%L}</b><br/>",
    valueDecimals: 2,
    style: {
      fontSize: "20px",
    },
  },

  title: {
    text: "Мотор-тестер",
    floating: true,
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
    useHTML: true,
    labelFormatter: function () {
      return `<table class="div-table">
            <tr class="div-table-row">
              <td class="div-table-col" align="center">${this.name}</td>
              <td class="div-table-col" align="center"></td>
            </tr>
            <tr class="div-table-row">
              <td class="div-table-col">Значение:</td>
              <td class="legend-value">${Highcharts.numberFormat(
                this.yData[this.yData.length - 1],
                1,
                "."
              )} ${this.userOptions.unit}</td>
            </tr>
          </table>`;
    },
  },
});

{
  for (var seriesConfig of seriesConfigs) {
    loadCellChart.addSeries({ ...seriesConfig });
    // loadCellChart.addSeries({ ...seriesConfigs[1], data: [1, 2] });
  }
}

const seriesPairs = [
  { sx: "rpm", sy: "power" },
  { sx: "rpm", sy: "loadCell" },
];

function drawGraphs() {
  for (let sp of seriesPairs) {
    

    let ng_xSeries = loadCellChart.series.find(
      (v) => v.userOptions.value_name == sp.sx
    );
    let ng_ySeries = loadCellChart.series.find(
      (v) => v.userOptions.value_name == sp.sy
    );

    let sname = `${sp.sx}_${sp.sy}`;
    let title = `${ng_xSeries.name}-${ng_ySeries.name}`;

    let ng_data = ng_xSeries.processedYData
      .map((v, i) => [v, ng_ySeries.processedYData[i]])
      .filter((v) => v[0] != -1);

    let newChart = Highcharts.chart(sname, {
      chart: {
        type: "scatter",
      },
      yAxis: { softMin: 0 },

      title: {
        text: title,
      },

      series: [
        { type: "scatter", name: title, data: ng_data },
        {
          type: "line",
          name: `${title}-тренд`,
          data: getTrendLine(ng_data),
        },
      ],
    });

    // newChart.addSeries({ name: newChart.title.textStr, data: ng_data });
  }
}

function getTrendLine(data) {
  const n = data.length;

  let sumX = 0,
    sumY = 0,
    sumXY = 0,
    sumX2 = 0;

  // Calculate the sums needed for linear regression
  for (let i = 0; i < n; i++) {
    const [x, y] = data[i];
    sumX += x;
    sumY += y;
    sumXY += x * y;
    sumX2 += x ** 2;
  }

  // Calculate the slope of the trend line
  const slope = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX ** 2);

  // Calculate the intercept of the trend line
  const intercept = (sumY - slope * sumX) / n;

  const trendline = []; // Array to store the trend line data points

  // Find the minimum and maximum x-values from the scatter plot data
  const minX = Math.min(...data.map(([x]) => x));
  const maxX = Math.max(...data.map(([x]) => x));

  // Calculate the corresponding y-values for the trend line using the slope
  // and intercept
  trendline.push([minX, minX * slope + intercept]);
  trendline.push([maxX, maxX * slope + intercept]);

  return trendline;
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

var j = 0;
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

  if (j == 0) loadCellChart.redraw(false);
  j++;
  j %= 2;
}

function onLoad(event) {
  initWebSocket();
}
