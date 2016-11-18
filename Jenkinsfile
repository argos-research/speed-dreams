node {
   stage('Preparation') { // for display purposes
      // Get some code from a GitHub repository
      // Could possibly be obsolete, will further investigate when isnan/inf bug is fixed
      git branch: 'KIA4SM', url: 'https://github.com/702nADOS/speed-dreams.git'  
      //Preparing build
      sh "mkdir -p build"
      dir('./build') {
         sh "cmake .."
        }
   }
   stage('Build') {
      // Run the build of SpeedDreams
         dir('./build') {
         sh "make -j"
        }
   }
   //Here tests or other stuff would appear
}
