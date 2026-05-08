import React from 'react';
import { Route, Routes } from 'react-router';
console.log('#app.tsx');

import Home from './client/home';
import About from './client/about';
import Login from './client/login';

function App() {
  return (
    <Routes>
      <Route path='/' element={<Home />} />
      <Route path="/about" element={<About />} />      
      <Route path="/login" element={<Login />} />      
    </Routes>
  );
}

export default App;
