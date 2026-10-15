import { useState } from 'react';
import { BrowserRouter, Routes, Route, Navigate } from 'react-router-dom';
import LoginPage from './pages/LoginPage';
import AdminDashboard from './pages/AdminDashboard';
import DriverDashboard from './pages/DriverDashboard';

function App() {
  const [user, setUser] = useState(null);

  const handleLogin = (userData) => {
    setUser(userData);
  };

  const handleLogout = () => {
    setUser(null);
  };


if (!user) {
  return (
    <BrowserRouter>
      <Routes>
        <Route
          path="*"
          element={
            <>
              <LoginPage onLogin={handleLogin} />
              <div
                style={{
                  position: 'fixed',
                  bottom: '8px',
                  right: '12px',
                  fontSize: '12px',
                  color: '#64748b',
                  background: '#ffffff',
                  padding: '6px 10px',
                  borderRadius: '6px',
                  zIndex: 9999,
                }}
              >
                DevOps Build: Automated Deployment
              </div>
            </>
          }
        />
      </Routes>
    </BrowserRouter>
  );
}


  return (
    <BrowserRouter>
      <Routes>
        {user.role === 'Admin' ? (
          <Route path="/*" element={<AdminDashboard user={user} onLogout={handleLogout} />} />
        ) : (
          <Route path="/*" element={<DriverDashboard user={user} onLogout={handleLogout} />} />
        )}
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </BrowserRouter>
  );
}

export default App;
