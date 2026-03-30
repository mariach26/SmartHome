import React, { useState, useEffect } from 'react';
import { FaFire, FaTint, FaWalking, FaVolumeUp, FaHistory, FaInfoCircle } from 'react-icons/fa';
import { ResponsiveContainer, AreaChart, Area, XAxis, YAxis, Tooltip, CartesianGrid } from 'recharts';
import './App.css';

// Τα στατικά δεδομένα για το γράφημα (μπορείς να τα αφήσεις έξω)
const data = [
  { time: '10:00', alerts: 1, details: [{ type: 'Κίνηση', room: 'Είσοδος' }] },
  { time: '11:00', alerts: 3, details: [{ type: 'Θόρυβος', room: 'Σαλόνι' }, { type: 'Νερό', room: 'Μπάνιο' }] },
  { time: '12:00', alerts: 2, details: [{ type: 'Κίνηση', room: 'Κουζίνα' }] },
  { time: '13:00', alerts: 5, details: [{ type: 'Φωτιά', room: 'Κουζίνα' }] },
  { time: '14:00', alerts: 0, details: [] },
];

function App() {
  const [hoveredData, setHoveredData] = useState(null);
  
  // 1. ΤΟ STATE ΠΡΕΠΕΙ ΝΑ ΕΙΝΑΙ ΕΔΩ ΜΕΣΑ
  const [liveSensors, setLiveSensors] = useState({
    water_percent: 0,
    fire_alarm: false,
    shock_alarm: false,
    radar_alarm: false,
    device: "Connecting..."
  });

  // 2. FETCH ΑΠΟ ΤΟ PYTHON BACKEND (FastAPI)
  const fetchSensors = async () => {
    try {
      const response = await fetch('http://localhost:8000/api/status');
      const data = await response.json();
      setLiveSensors(data);
    } catch (error) {
      console.error("Backend offline...", error);
    }
  };

  useEffect(() => {
    const interval = setInterval(fetchSensors, 2000); // Ανανέωση κάθε 2 δευτερόλεπτα
    return () => clearInterval(interval);
  }, []);

  const CustomTooltip = ({ active, payload }) => {
    if (active && payload && payload.length) {
      const currentPoint = payload[0].payload;
      if (!hoveredData || hoveredData.time !== currentPoint.time) {
        setHoveredData(currentPoint);
      }
    }
    return null;
  };

  return (
    <div className="dashboard-container">
      <header>
        <h1>Smart Home IoT <span className="live-badge">LIVE</span></h1>
        <p style={{color: '#94a3b8', fontSize: '0.8rem'}}>Device: {liveSensors.device}</p>
      </header>

      <div className="sensor-grid">
        {/* ΚΑΡΤΑ ΦΩΤΙΑΣ - Γίνεται κόκκινη στο Alarm */}
        <div className={`sensor-card ${liveSensors.fire_alarm ? 'critical-alert' : ''}`}>
          <FaFire className="icon fire-icon" />
          <h3>Φωτιά</h3>
          <span className="sensor-value">
            {liveSensors.fire_alarm ? "🔥 ALARM" : "Safe"}
          </span>
        </div>

        {/* ΚΑΡΤΑ ΝΕΡΟΥ */}
        <div className="sensor-card">
          <FaTint className="icon water-icon" />
          <h3>Νερό</h3>
          <span className="sensor-value">{liveSensors.water_percent}%</span>
        </div>

        {/* ΚΑΡΤΑ ΚΙΝΗΣΗΣ */}
        <div className="sensor-card">
          <FaWalking className="icon motion-icon" />
          <h3>Κίνηση</h3>
          <span className="sensor-value">
            {liveSensors.radar_alert ? "⚠️ MOTION" : "No Motion"}
          </span>
        </div>

        {/* ΚΑΡΤΑ ΚΡΑΔΑΣΜΟΥ */}
        <div className="sensor-card">
          <FaVolumeUp className="icon sound-icon" />
          <h3>Κραδασμός</h3>
          <span className="sensor-value">
            {liveSensors.shock_alarm ? "📳 SHOCK" : "Stable"}
          </span>
        </div>
      </div>

      <div className="chart-section">
        <h2>Activity Monitor</h2>
        <div className="chart-container" onMouseLeave={() => setHoveredData(null)}>
          <ResponsiveContainer width="100%" height={250}>
            <AreaChart data={data}>
              <defs>
                <linearGradient id="colorAlerts" x1="0" y1="0" x2="0" y2="1">
                  <stop offset="5%" stopColor="#38bdf8" stopOpacity={0.8}/>
                  <stop offset="95%" stopColor="#38bdf8" stopOpacity={0}/>
                </linearGradient>
              </defs>
              <CartesianGrid strokeDasharray="3 3" stroke="#334155" vertical={false} />
              <XAxis dataKey="time" stroke="#94a3b8" />
              <YAxis stroke="#94a3b8" />
              <Tooltip content={<CustomTooltip />} /> 
              <Area 
                type="monotone" 
                dataKey="alerts" 
                stroke="#38bdf8" 
                strokeWidth={3}
                fillOpacity={1} 
                fill="url(#colorAlerts)" 
                activeDot={{ r: 8 }}
              />
            </AreaChart>
          </ResponsiveContainer>
        </div>

        <div className="hover-details-box" style={{ border: hoveredData ? '1px solid #38bdf8' : '1px solid #334155' }}>
          {hoveredData ? (
            <div>
              <div className="box-header">
                <FaInfoCircle style={{color: '#38bdf8'}} />
                <h3 style={{margin: 0}}>Στις {hoveredData.time} είχαμε {hoveredData.alerts} συμβάντα:</h3>
              </div>
              <div className="details-scroll" style={{display: 'flex', gap: '10px', marginTop: '10px', flexWrap: 'wrap'}}>
                {hoveredData.alerts > 0 ? (
                  hoveredData.details.map((d, i) => (
                    <div key={i} className="mini-detail-item">
                      <span style={{color: '#38bdf8'}}>●</span> {d.type} ({d.room})
                    </div>
                  ))
                ) : (
                  <p style={{color: '#94a3b8'}}>Κανένα πρόβλημα αυτή την ώρα.</p>
                )}
              </div>
            </div>
          ) : (
            <div className="placeholder-text">
              <FaHistory /> Πέρασε το ποντίκι πάνω από το γράφημα για λεπτομέρειες
            </div>
          )}
        </div>
      </div>
    </div>
  );
}

export default App;
