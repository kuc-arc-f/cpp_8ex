//import { Routes, Route, Link } from 'react-router-dom';
import {Link } from 'react-router-dom';
import React, { useState , useEffect } from 'react';
import LibConfig from '../client/lib/LibConfig';
import LibCookie from '../client/lib/LibCookie';

function Page() {
  const proc_logout = () =>{
    console.log("Logout")
    const key = LibConfig.COOKIEK_KEY_USER;
    LibCookie.delete_cookie(key);
    location.href = "/login";
  }

  return (
  <div class="flex flex-row">
    <div class="flex-1 p-2 m-1">
      <a href="/" className="font-bold ms-4" >Home</a>
      <a href="/about" className="ms-4" >About</a>
    </div>
    <div class="flex-1 p-2 m-1 text-end">
      <button onClick={() => proc_logout()}>[ Logout ]</button>
    </div>    
  </div>
  );
}
export default Page;
// <hr className="my-2" />


