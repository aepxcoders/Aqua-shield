// const backendURL = "http://127.0.0.1:5000/data";

const backendURL = "https://aqua-shield-5jam.onrender.com";

function setStatusUI(status) {
  const banner = document.getElementById("banner");
  banner.classList.remove("normal", "warning", "danger");

  if (status === "Normal") banner.classList.add("normal");
  else if (status === "Monitor") banner.classList.add("warning");
  else banner.classList.add("danger");
}

function getWaterType(turbidity) {
  if (turbidity < 1200) return "Clean Water";
  if (turbidity < 2500) return "Mixed Water";
  return "Contaminated Water";
}

function getQualityLevel(turbidity) {
  if (turbidity < 1200) return "Low Risk";
  if (turbidity < 2500) return "Moderate Risk";
  return "High Risk";
}

async function fetchData() {
  try {
    const res = await fetch(backendURL);
    const data = await res.json();

    console.log("Backend data:", data);

    const flow1 = Number(data.flow1 ?? data.inletFlow ?? 0);
    const flow2 = Number(data.flow2 ?? data.outletFlow ?? 0);

    let loss = Number(data.loss ?? data.flowDifference ?? 0);
    if (loss < 0) loss = 0;

    const flowLossPercent = Number(
      data.flowLossPercent ?? (flow1 > 0 ? (loss / flow1) * 100 : 0),
    );

    const sourceDrop = Number(data.sourceDrop ?? data.inletDistance ?? 0);
    const outletRise = Number(data.outletRise ?? data.outletDistance ?? 0);

    // FINAL FIX: mismatch is calculated from visible dashboard values
    const levelMismatch = Math.abs(sourceDrop - outletRise);

    const turbidity = Number(data.turbidity ?? data.turbidityValue ?? 0);
    const waterType = data.waterType || getWaterType(turbidity);
    const qualityLevel = data.qualityLevel || getQualityLevel(turbidity);

    const status = data.status || "Normal";
    const recommendation =
      data.recommendation ||
      "Readings received. Continue monitoring based on current alert level.";

    document.getElementById("flow1").innerText = flow1.toFixed(2);
    document.getElementById("flow2").innerText = flow2.toFixed(2);
    document.getElementById("loss").innerText = loss.toFixed(2);

    document.getElementById("flowLossPercent").innerText =
      flowLossPercent.toFixed(1) + "%";

    document.getElementById("sourceDrop").innerText = sourceDrop.toFixed(2);
    document.getElementById("outletRise").innerText = outletRise.toFixed(2);

    // update BOTH mismatch places
    document.getElementById("levelMismatch").innerText =
      levelMismatch.toFixed(2) + " cm";

    if (document.getElementById("levelMismatchBox")) {
      document.getElementById("levelMismatchBox").innerText =
        levelMismatch.toFixed(2);
    }

    document.getElementById("turbidity").innerText = turbidity;
    document.getElementById("waterType").innerText = waterType;
    document.getElementById("qualityLevel").innerText = qualityLevel;

    document.getElementById("status").innerText = status;
    document.getElementById("statusText").innerText = recommendation;
    document.getElementById("recommendation").innerText = recommendation;

    if (document.getElementById("levelMessage")) {
      document.getElementById("levelMessage").innerText =
        levelMismatch > 10
          ? "Mismatch is above the allowed margin of 10 cm."
          : "All readings are within the 10 cm margin.";
    }

    setStatusUI(status);
  } catch (e) {
    console.log("Error fetching:", e);
  }
}

setInterval(fetchData, 1000);
fetchData();
