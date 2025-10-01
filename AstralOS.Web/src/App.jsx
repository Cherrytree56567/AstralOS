import React from "react";
import Particles from "./Particles";

export default function App() {
  return (
    <div style={{ width: "100%", height: "600px" }}>
      <Particles
        particleColors={["#fff", "#fff"]}
        particleCount={200}
        particleSpread={10}
        speed={0.1}
        particleBaseSize={100}
        moveParticlesOnHover={true}
        alphaParticles={false}
        disableRotation={false}
      />
    </div>
  );
}
