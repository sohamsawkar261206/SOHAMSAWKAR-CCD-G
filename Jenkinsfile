
pipeline {
    agent any

    options {
        timestamps()
    }

    stages {
        stage('Checkout') {
            steps {
                checkout scm
            }
        }

        stage('Build Docker Image') {
            steps {
                sh 'docker build -t routemaster-cca2:${BUILD_NUMBER} .'
            }
        }

        stage('Verify Image') {
            steps {
                sh 'docker image inspect routemaster-cca2:${BUILD_NUMBER}'
                echo 'Docker image built and verified successfully.'
            }
        }
    }

    post {
        success {
            echo 'RouteMaster CCA2 pipeline completed successfully.'
        }
        failure {
            echo 'Pipeline failed. Check the Console Output for details.'
        }
    }
}
